# re_nfpii
[![](https://dcbadge.vercel.app/api/server/geY4G2NZK9?style=flat&compact=true)](https://discord.gg/geY4G2NZK9)

A more or less accurate reimplementation of the Wii U's nn_nfp.rpl library for the [Wii U Plugin System](https://github.com/wiiu-env/WiiUPluginSystem), with the goal of research and amiibo emulation.  
Requires the [Aroma](https://github.com/wiiu-env/Aroma) environment.

> :warning: This is still really experimental and work-in-progress!  
Check the [compatibility list](https://github.com/GaryOderNichts/re_nfpii/wiki/Compatibility-List) for working games. Expect potential data loss of amiibo data.

## Usage
- Download the artifacts of latest nightly build from [here](https://github.com/GaryOderNichts/re_nfpii/actions) (Requires a GitHub account).
- Copy the `re_nfpii.wps` to the `plugins` folder of your target environment.
- Copy your (encrypted!) amiibo dumps to `wiiu/re_nfpii`.
- Open the plugin configuration menu with L + Down + SELECT.
- Select one of your amiibo and enable emulation.

### Remove after
- The remove after feature "removes" the virtual tag from the reader after the specified amount of time.

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
