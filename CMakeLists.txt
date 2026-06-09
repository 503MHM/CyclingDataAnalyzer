cmake_minimum_required(VERSION 3.19)
project(CyclingDataAnalyzer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Sql)

qt_standard_project_setup()

qt_add_executable(CyclingDataAnalyzer
    #WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.ui
    
    src/network/OnenetClient.h
    src/network/OnenetClient.cpp
    src/network/OnenetTypes.h
    src/database/AppDatabase.h
    src/database/AppDatabase.cpp
    src/domain/Device.h
    src/database/DeviceRepository.h
    src/database/DeviceRepository.cpp
    src/domain/Ride.h
    src/database/RideRepository.h
    src/database/RideRepository.cpp

    src/domain/SensorSample.h
    src/database/SensorDataRepository.h
    src/database/SensorDataRepository.cpp
    src/domain/RideEvent.h
    src/database/EventRepository.cpp
    src/database/EventRepository.h
    src/service/EventMapper.cpp
    src/service/EventMapper.h
    src/service/SensorSampleBuilder.h
    src/service/SensorSampleBuilder.cpp
    src/service/AnalysisService.h
    src/service/AnalysisService.cpp
    src/service/SyncService.h
    src/service/SyncService.cpp
    src/service/RideBuilder.h
    src/service/RideBuilder.cpp
    src/service/SyncWorker.cpp
    src/service/SyncWorker.h
)

target_link_libraries(CyclingDataAnalyzer
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt6::Sql
)

target_include_directories(CyclingDataAnalyzer PRIVATE src)

install(TARGETS CyclingDataAnalyzer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET CyclingDataAnalyzer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
