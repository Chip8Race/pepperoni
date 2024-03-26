#pragma once

#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/screen_interactive.hpp"

#include <algorithm>
#include <ctime>
#include <fmt/chrono.h>
#include <memory>
#include <string>

#include "events.hpp"

// TODO: Remove this from header
using namespace ftxui;

namespace peppe {

struct Msg {
    std::string username;
    std::string content;
    std::tm time;
};

class Frontend : public EventListener<Frontend, BackendEvent> {

public:
    // Ctor
    Frontend()
        : m_input_component(Input(&m_input_message, "Write something"))
        , m_component(Container::Vertical({
              m_input_component,
          })) {
        m_renderer = Renderer(m_component, [this] {
            // Message component
            auto msg_comp = [](Msg const& msg) {
                auto time_txt = fmt::format("  {:%H:%M}", msg.time);
                return hbox(
                    { separatorEmpty(),
                      vbox({ hbox({ text(msg.username) | bold,
                                    text(time_txt) | color(Color::GrayDark) }),
                             paragraph(msg.content),
                             separatorEmpty() }) }
                );
            };

            // Create message list component
            std::vector<Element> msgs_comp;
            for (auto const& msg : m_history) {
                msgs_comp.emplace_back(msg_comp(msg));
            }
            if (msgs_comp.size() > 0) {
                msgs_comp.back() |= focus;
            }

            // Return ui
            return vbox({
                       vbox(std::move(msgs_comp)) | flex | frame,
                       separator(),
                       hbox(text(" Message : "), m_input_component->Render()),
                   }) |
                   borderHeavy;
        });
    }

    // Copy
    Frontend(Frontend const&) = delete;
    Frontend& operator=(Frontend const&) = delete;
    // Move
    Frontend(Frontend&&) = delete;
    Frontend& operator=(Frontend&&) = delete;
    // Dtor
    ~Frontend() = default;

    bool on_event(const ftxui::Event& event) {
        if (event == ftxui::Event::Escape) {
            m_screen.ExitLoopClosure()();
            return true;
        }
        else if (event == ftxui::Event::Return) {
            // FrontendEvent tmp{ SendMessage{} };
            EventManager::send(FrontendEvent{ SendMessage{ m_input_message } });
            auto current_epoch = std::time(nullptr);
            m_history.emplace_back(
                "Jojo",
                std::move(m_input_message),
                std::move(*std::localtime(&current_epoch))
            );
            m_input_message = "";
            return false;
        }
        return false;
    }

    void on_event(const BackendEvent& event) override {
        event.match(
            [this](const ReceiveMessage& sm) {
                auto current_epoch = std::time(nullptr);
                m_history.emplace_back(
                    sm.from,
                    sm.message,
                    std::move(*std::localtime(&current_epoch))
                );
            },
            [](const auto&) {}
        );
        // Explicit redraw trigger
        m_screen.PostEvent(ftxui::Event::Custom);
    }

    void start() {
        m_screen.Loop(CatchEvent(m_renderer, [this](ftxui::Event event) {
            return on_event(event);
        }));
    }

private:
    std::string m_input_message;
    std::vector<Msg> m_history = {};
    Component m_input_component;
    Component m_component;
    Component m_renderer;
    ScreenInteractive m_screen = ScreenInteractive::Fullscreen();
};

} // namespace peppe