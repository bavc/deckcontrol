# deckcontrol

A utility to facilitate deck control via Blackmagic SDK.

maintainer: @dericed.

## installation

### via homebrew

Homebrew is recommended to install deckcontrol:

`brew install amiaopensource/amioas/deckcontrol`

### to build from source

The BMDSK variable must be set to the path of the Blackmagic Decklink SDK. The SDK is available at https://www.blackmagicdesign.com/support/ as "Desktop Video #{version} SDK".

```
make BMDSK=/path/to/decklink/sdk
```
