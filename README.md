# My-DJI-OnboardSDK
Purpose
The current DJI OnboardSDK sample uses the C programming style for compatibility, which introduces inconvenience, especially when user is using C++. For example, there is no elegant way to set a non-static member function as a callback of the OnboardSDK. 
The other reason for this projec is that I am not satisfied with the framework design of the current DJI OnboardSDK sample. Originally, the OnboardSDK sample is used to show the usage of OnboardSDK. It is not supposed to be used directly as a library for other projects. However, it seems that most of the users use the sample directly for their projects. 
