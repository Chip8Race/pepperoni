#pragma once

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"

#include <ctime>
#include <fmt/chrono.h>
#include <string>

#include "events.hpp"

namespace peppe {

struct Msg {
    std::string username;
    std::string content;
    std::tm time;
    bool is_me = false;
};

class Frontend : public EventListener<Frontend, BackendEvent> {

public:
    // Ctor
    Frontend(std::string&& name);
    // Copy
    Frontend(Frontend const&) = delete;
    Frontend& operator=(Frontend const&) = delete;
    // Move
    Frontend(Frontend&&) = delete;
    Frontend& operator=(Frontend&&) = delete;
    // Dtor
    ~Frontend() = default;

    bool on_event(const ftxui::Event& event);

    void on_event(const BackendEvent& event) override;

    void start();

private:
    std::string m_input_message;
    std::string m_client_name;
    std::vector<Msg> m_history = {};
    ftxui::Component m_input_component;
    ftxui::Component m_component;
    ftxui::Component m_renderer;
    ftxui::ScreenInteractive m_screen = ftxui::ScreenInteractive::Fullscreen();
};

} // namespace peppe