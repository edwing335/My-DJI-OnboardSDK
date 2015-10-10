# My-DJI-OnboardSDK
## Purpose
The current DJI OnboardSDK sample uses the C programming style for compatibility, which introduces inconvenience, especially when user is using C++. For example, there is no elegant way to set a non-static member function as a callback of the OnboardSDK. <br>
The other reason for this project is that I am not satisfied with the framework design of the current DJI OnboardSDK sample. Originally, the OnboardSDK sample is used to show the usage of OnboardSDK. It is not supposed to be used directly as a library for other projects. However, it seems that most of the users use the sample directly for their projects. <br><br>
So I decide to re-write the sample. The new version should work on different platforms, Linux, Mac and Windows, as long as you can find a compiler supporting C++11. The structure of the program should be well-designed and the interface should be convenient. <br><br>
## Status
I am already working on the project. Now the program supports activation, take-off, land and go home. More functions will be supported in the future. 
