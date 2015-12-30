# Hacktivité

This repo contains two small programs - _basefind_ and _firmware\_verification_.

_Basefind_, a rewrite of [@mncoppola's](https://twitter.com/mncoppola) python version, can be used to find the likely load address of a firmware or other embedded binary. It tries to find the load address that results in the most strings being referenced from the resulting disassembly.

_Firmware\_verification_ does exactly what it says: It shows how firmware updates for Withings' Activité are being verified. Perhaps you have noticed that this verification is not really all that complicated. That is why this little tool also allows you to _resign_ firmware images you modified. Beware though, the tracker might just accept any firmware with a valid header, so make sure it won't brick your device.

This work is part of a university project. Stay tuned for the paper describing the results!