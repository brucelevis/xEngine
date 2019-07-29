#include "InputEvent.h"

NS_FGUI_BEGIN

InputEvent::InputEvent() :
    _target(nullptr),
    _touch(nullptr),
    _inputProcessor(nullptr),
    _touchId(-1),
    _clickCount(0),
    _mouseWheelDelta(0),
    _button(MouseButton::BUTTON_UNSET),
    _keyCode(KeyCode::KEY_0),
    _keyModifiers(0)
{
}

InputEvent::~InputEvent()
{

}


NS_FGUI_END