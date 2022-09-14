# re_nfpii
[![](https://dcbadge.vercel.app/api/server/geY4G2NZK9?style=flat&compact=true)](https://discord.gg/geY4G2NZK9)

A more or less accurate reimplementation of the Wii U's nn_nfp.rpl library, with the goal of research and amiibo emulation.  
Requires the [Aroma](https://github.com/wiiu-env/Aroma) environment.  

> :warning: This is still really experimental and work-in-progress!  
Check the [compatibility list](https://github.com/GaryOderNichts/re_nfpii/wiki/Compatibility-List) for working games. Expect potential data loss of amiibo data.

re_nfpii is split into a module and a plugin.  
The module completely replaces all exports of nn_nfp and adds additional exports for configuration. A majority of the code tries to be accurate to what nn_nfp is doing, so don't question some of the design choices.  
The plugin handles the configurarion menu for the module and some additional things which can't be added to the module.

## Usage
- Download the [latest release](https://github.com/GaryOderNichts/re_nfpii/releases) or the artifacts of latest nightly build from [here](https://github.com/GaryOderNichts/re_nfpii/actions) (Requires a GitHub account).
- Copy the contents of the downloaded *`.zip`* file to your target environment.
- Copy your (encrypted!) amiibo dumps to `wiiu/re_nfpii`.
- Open the plugin configuration menu with L + Down + SELECT.
- Select one of your amiibo and enable emulation.

### Remove after
- The remove after feature "removes" the virtual tag from the reader after the specified amount of time.

### Amiibo Settings
The Wii U Plugin System does not work in applets, such as the Amiibo Settings, yet.  
This means you can't open the re_nfpii configuration while in the Amiibo Settings.  
To use the Amiibo Settings, select the Amiibo you want to configure before entering them, and everything else will be handled automatically.  
After returning from the Amiibo Settings, re-enable Amiibo emulation.

## Planned features
This currently just reimplements the major parts of nn_nfp and redirects tag reads and writes to the SD Card.  
For future releases it is planned to have additional features and a custom format which does not require encryption.

## Building
This currently uses a custom build of [wut](https://github.com/GaryOderNichts/wut/tree/re_nfpii) until I'll make a PR with all of the missing error descriptions and structs.  
I recommend building using the Dockerfile:
```
# Build docker image (only needed once)
docker build . -t re_nfpii_builder

# make 
docker run -it --rm -v ${PWD}:/project re_nfpii_builder make

# make clean
docker run -it --rm -v ${PWD}:/project re_nfpii_builder make clean
```
