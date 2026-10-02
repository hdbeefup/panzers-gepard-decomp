# window — UI widget library
add_library(window STATIC
    button.cpp
    checkbox.cpp
    colorbutton.cpp
    credits.cpp
    demoexit.cpp
    dialog.cpp
    droplist.cpp
    dxwidget.cpp
    dxwindow.cpp
    editbox.cpp
    editboxwithtext.cpp
    filelistbox.cpp
    inputdialog.cpp
    introduction.cpp
    label.cpp
    listbox.cpp
    matchlistbox.cpp
    messagebox.cpp
    messageboxwithoutbuttons.cpp
    playerlistbox.cpp
    radiobutton.cpp
    saveloadlistbox.cpp
    scaler.cpp
    skippabledxwidget.cpp
    sliderh.cpp
    sliderv.cpp
    splash.cpp
    textbox.cpp
    textbutton.cpp
    tipdialog.cpp
    widget.cpp
    window.cpp
)

target_include_directories(window PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(window PRIVATE ${CMAKE_SOURCE_DIR}/sound)
target_link_libraries(window PUBLIC core 3dengine common)
