#include "frontend.hpp"
#include "fmt/base.h"
#include <ftxui/screen/color.hpp>

using namespace ftxui;

namespace peppe {

Frontend::Frontend(std::string&& name)
    : m_client_name(std::move(name))
    , m_input_component(Input(&m_input_message, "Write something"))
    , m_component(Container::Vertical({
          m_input_component,
      })) {
    m_renderer = Renderer(m_component, [this] {
        // Message component
        auto msg_comp = [](const Msg& msg) {
            auto time_txt = fmt::format("  {:%H:%M}", msg.time);
            // fmt::print(stderr, "{}: is_me({})\n", msg.username, msg.is_me);
            auto name_text = text(msg.username) | bold;
            if (msg.is_me) {
                name_text |= color(Color::Yellow);
            }
            return hbox(
                { separatorEmpty(),
                  vbox({ hbox({ name_text,
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

bool Frontend::on_event(const ftxui::Event& event) {
    if (event == ftxui::Event::Escape) {
        m_screen.ExitLoopClosure()();
        return true;
    }
    else if (event == ftxui::Event::Return) {
        EventManager::send(FrontendEvent{ SendMessage{ m_input_message } });
        auto current_epoch = std::time(nullptr);
        m_history.emplace_back(
            m_client_name,
            std::move(m_input_message),
            *std::localtime(&current_epoch),
            true
        );
        m_input_message = "";
        return false;
    }
    return false;
}

void Frontend::on_event(const BackendEvent& event) {
    event.match(
        [this](const ReceiveMessage& sm) {
            auto current_epoch = std::time(nullptr);
            m_history.emplace_back(
                sm.from, sm.message, *std::localtime(&current_epoch), false
            );
        },
        [](const auto&) {}
    );
    // Explicit redraw trigger
    m_screen.PostEvent(ftxui::Event::Custom);
}

void Frontend::start() {
    m_screen.Loop(CatchEvent(m_renderer, [this](ftxui::Event event) {
        return on_event(event);
    }));
}

} // namespace peppe