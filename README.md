# KinectUE4

Kinect2.0 SDK for Windows接入UE4的一个插件。
此工程只是一个gist工程，其代码展示了骨架识别、关节识别、动作识别相关API的使用方法。

# Requirements

1. UnrealEngine 4.20 
2. 拷贝 
    - Kinect20.dll -> {ProjectDir}/Binaries/Win64/Kinect20.dll
    - Kinect20.VisualGestureBuilder.dll -> {ProjectDir}/Binaries/Win64/Kinect20.VisualGestureBuilder.dll
    - vgbtechs -> /Binaries/Win64/vgbtechs
3. 修改插件工程.Build文件中关于.lib引用的路径