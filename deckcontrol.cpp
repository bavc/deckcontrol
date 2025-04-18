#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "DeckLinkAPI.h"
#include "common.h"

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

enum DeckCommand {
    NO_COMMAND = -1,
    GET_CURRENT_STATE,
    SET_STANDBY,
    PLAY,
    STOP,
    TOGGLE_PLAY_STOP,
    EJECT,
    GO_TO_TIMECODE,
    FAST_FORWARD,
    REWIND,
    STEP_FORWARD,
    STEP_BACK,
    JOG,
    SHUTTLE,
    GET_TIMECODE,
    CRASH_RECORD_START,
    CRASH_RECORD_STOP
};

struct DeckCommandMapping
{
    /**
     * Name of the command, as specified on the command line
     */
    const char *commandString;

    /**
     * Corresponding of enum DeckCommand
     */
    DeckCommand commandNum;

    /**
     * Number of additional parameters required for command
     */
    int numParameters;
};

static const DeckCommandMapping commandTable [] = {
    { "getcurrentstate", GET_CURRENT_STATE, 0 },
    { "play", PLAY, 0 },
    { "stop", STOP, 0 },
    { "toggleplaystop", TOGGLE_PLAY_STOP, 0 },
    { "eject", EJECT, 0 },
    { "gototimecode", GO_TO_TIMECODE, 1 },
    { "fastforward", FAST_FORWARD, 0 },
    { "rewind", REWIND, 0 },
    { "stepforward", STEP_FORWARD, 0 },
    { "stepback", STEP_BACK, 0 },
    { "jog", JOG, 1 },
    { "shuttle", SHUTTLE, 1 },
    { "gettimecode", GET_TIMECODE, 0 },
    { "crashrecordstart", CRASH_RECORD_START, 0 },
    { "crashrecordstop", CRASH_RECORD_STOP, 0 },
    { NULL }
};

class StatusCallback : public IDeckLinkDeckControlStatusCallback
{
private:
    pthread_mutex_t *m_mutex;
    pthread_cond_t *m_cond;
    bool m_connected;

public:
    StatusCallback(pthread_mutex_t *mutex, pthread_cond_t *cond) :
        m_mutex(mutex),
        m_cond(cond),
        m_connected(false) {}

    virtual HRESULT TimecodeUpdate (BMDTimecodeBCD currentTimecode) { return E_NOTIMPL; }
    virtual HRESULT VTRControlStateChanged (BMDDeckControlVTRControlState newState, BMDDeckControlError error) { return E_NOTIMPL; }
    virtual HRESULT DeckControlEventReceived (BMDDeckControlEvent event, BMDDeckControlError error) { return E_NOTIMPL; }
    virtual HRESULT DeckControlStatusChanged (BMDDeckControlStatusFlags flags, uint32_t mask)
    {
        pthread_mutex_lock(m_mutex);
        if ((mask & bmdDeckControlStatusDeckConnected) && (flags & bmdDeckControlStatusDeckConnected))
        {
            m_connected = true;
            pthread_cond_signal(m_cond);
        }
        pthread_mutex_unlock(m_mutex);

        return S_OK;
    }

    HRESULT QueryInterface (REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
    ULONG AddRef ()  { return 1; }
    ULONG Release () { return 1; }

    bool isConnected() { return m_connected; }
};

int main(int argc, char *argv[])
{
    int err = 0;
    DeckCommand cmd = NO_COMMAND;
    const char *cmdarg;
    const DeckCommandMapping *tmpTable;

    IDeckLinkIterator *deckLinkIterator = NULL;
    IDeckLink *deckLink = NULL;
    IDeckLinkDeckControl *deckControl = NULL;
    BMDDeckControlError deckError = bmdDeckControlNoError;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    StatusCallback callback(&mutex, &cond);

    // Parse command line

    if (argc > 1)
    {
        tmpTable = commandTable;
        while (tmpTable->commandString)
        {
            if (!strcmp(argv[1], tmpTable->commandString) &&
                argc - 2 >= tmpTable->numParameters)
            {
                cmd = tmpTable->commandNum;
                if (argc > 2)
                    cmdarg = argv[2];
                else
                    cmdarg = "0";
                printf("Issued command '%s'\n", tmpTable->commandString);
            }
            tmpTable++;
        }
    }

    // Print usage if command line is invalid

    if (cmd == NO_COMMAND)
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "%s <command> [parameter1] [parameter2] ...\n\n", argv[0]);
        fprintf(stderr, "Commands:\n");
        tmpTable = commandTable;
        while (tmpTable->commandString)
        {
            printf("  %s", tmpTable->commandString);
            for (int i=1; i<=tmpTable->numParameters; i++)
                printf(" [parameter %d]", i);
            printf("\n");
            tmpTable++;
        }
        err = -1;
        goto bail;
    }

    // Try to load decklink driver and open the first card

    deckLinkIterator = CreateDeckLinkIteratorInstance();

    if (!deckLinkIterator)
    {
        fprintf(stderr, "This application requires the DeckLink drivers installed\n");
        err = -2;
        goto bail;
    }

    if (deckLinkIterator->Next(&deckLink) != S_OK)
    {
        fprintf(stderr, "Could not detect a DeckLink card\n");
        err = -4;
        goto bail;
    }

    if (deckLink->QueryInterface(IID_IDeckLinkDeckControl, (void **)&deckControl) != S_OK)
    {
        fprintf(stderr, "Could not obtain the DeckControl interface\n");
        err = -8;
        goto bail;
    }

    // Connect to deck

    deckControl->SetCallback(&callback);

    pthread_mutex_lock(&mutex);
    if (deckControl->Open(30000, 1001, true, &deckError) == S_OK)
    {
        while (!callback.isConnected())
            pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        pthread_mutex_unlock(&mutex);
        fprintf(stderr, "Could not open serial port (%s)\n", ERR_TO_STR(deckError));
        err = -16;
        goto bail;
    }

    // Send the command

    switch(cmd)
    {
    case GET_CURRENT_STATE:
        BMDDeckControlMode mode;
        BMDDeckControlVTRControlState vtrControlState;
        BMDDeckControlStatusFlags flags;
        deckControl->GetCurrentState(&mode, &vtrControlState, &flags);
        printf("VTR control state: %s\n", STATE_TO_STR(vtrControlState));
        break;
    case PLAY:
        deckControl->Play(&deckError);
        break;
    case STOP:
        deckControl->Stop(&deckError);
        break;
    case TOGGLE_PLAY_STOP:
        deckControl->TogglePlayStop(&deckError);
        break;
    case EJECT:
        deckControl->Eject(&deckError);
        break;
    case GO_TO_TIMECODE:
        int tc[4];
        cmdarg = strtok(argv[2], ":");
        for (int i=0; i<4; i++)
        {
            if (!cmdarg) break;
            tc[i] = atoi(cmdarg);
            if (tc[i] > 59 || tc[i] < 0)
                tc[i] = 0;
            cmdarg = strtok(NULL, ":");
        }
        deckControl->GoToTimecode(MAKE_TC_BCD(tc[0] / 10, tc[0] % 10,
                                              tc[1] / 10, tc[1] % 10,
                                              tc[2] / 10, tc[2] % 10,
                                              tc[3] / 10, tc[3] % 10),
                                  &deckError);
        break;
    case FAST_FORWARD:
        deckControl->FastForward(atoi(cmdarg), &deckError);
        break;
    case REWIND:
        deckControl->Rewind(atoi(cmdarg), &deckError);
        break;
    case STEP_FORWARD:
        deckControl->StepForward(&deckError);
        break;
    case STEP_BACK:
        deckControl->StepBack(&deckError);
        break;
    case JOG:
        deckControl->Jog(atof(argv[2]), &deckError);
        break;
    case SHUTTLE:
        deckControl->Shuttle(atof(argv[2]), &deckError);
        break;
    case GET_TIMECODE:
        IDeckLinkTimecode *currentTimecode;
        uint8_t hours, minutes, seconds, frames;
        deckControl->GetTimecode(&currentTimecode, &deckError);
        currentTimecode->GetComponents(&hours, &minutes, &seconds, &frames);
        printf("TC=%02" PRIu8 ":%02" PRIu8 ":%02" PRIu8 ":%02" PRIu8 "\n",
               hours, minutes, seconds, frames);
        SAFE_RELEASE(currentTimecode);
        break;
    case CRASH_RECORD_START:
        deckControl->CrashRecordStart(&deckError);
        break;
    case CRASH_RECORD_STOP:
        deckControl->CrashRecordStop(&deckError);
        break;
    }

    if (deckError == bmdDeckControlNoError)
    {
        printf("Command sucessfully issued\n");
    }
    {
        printf("Error sending command (%s)\n", ERR_TO_STR(deckError));
    }

    deckControl->Close(false);

bail:
    if (deckControl)
        deckControl->SetCallback(NULL);
    SAFE_RELEASE(deckControl);
    SAFE_RELEASE(deckLink);
    SAFE_RELEASE(deckLinkIterator);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    return err;
}
