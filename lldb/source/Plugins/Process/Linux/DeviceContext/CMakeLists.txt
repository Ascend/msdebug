add_lldb_library(lldbDeviceContext
    DeviceContext.cpp
    Ascend910BDeviceContext.cpp
    Ascend310PDeviceContext.cpp
    Ascend910DDeviceContext.cpp

    LINK_LIBS
      lldbHost
    LINK_COMPONENTS
      Support
)
