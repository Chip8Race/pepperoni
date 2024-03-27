#pragma once

#include "utils.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <typeindex>

namespace peppe {

///////////////////////////////
// Events                    //
///////////////////////////////

struct Event {};

template<typename... E>
struct CompoundEvent
    : public Variant<E...>
    , public Event {

    template<typename K>
        requires(std::is_same_v<K, E> || ...)
    CompoundEvent(K&& event)
        : Variant<E...>(event) {}

    template<typename... F>
    void match(F&&... funcs) const {
        std::visit(overloaded<F...>{ std::forward<F>(funcs)... }, *this);
    }
};

struct ReceiveMessage {
    std::string from;
    std::string message;
};

struct SetPeerName {
    std::string name;
};

struct PeerConnected {};

struct PeerDisconnected {};

using BackendEvent =
    CompoundEvent<ReceiveMessage, SetPeerName, PeerConnected, PeerDisconnected>;

struct SendMessage {
    std::string message;
};

struct Terminate {};

using FrontendEvent = CompoundEvent<SendMessage, Terminate>;

//////////////////////////////////
// Event handler                //
//////////////////////////////////

template<typename E>
concept IsEvent = std::is_base_of_v<Event, E>;

template<typename T, typename E>
concept HasOnEvent = requires(T t, E e) { t.on_event(e); };

class EventHandler {
public:
    using HandlerID = std::uintptr_t;

    template<typename T, IsEvent E>
    explicit EventHandler(T* instance, void (T::*func)(E const&))
        : handler_id{ id_counter++ }
        , event_type_id{ typeid(E) }
        , handler{ [instance, func](Event const& event) {
            return (instance->*func)(static_cast<const E&>(event));
        } } {}

    template<IsEvent E>
    explicit EventHandler(void (*func)(E const&))
        : handler_id{ id_counter++ }
        , event_type_id{ typeid(E) }
        , handler{ [func](Event const& event) {
            return (*func)(static_cast<const E&>(event));
        } } {}

    // Copy
    EventHandler(EventHandler const&) = default;
    EventHandler& operator=(EventHandler const&) = default;
    // Move
    EventHandler(EventHandler&&) = default;
    EventHandler& operator=(EventHandler&&) = default;
    // Dtor
    ~EventHandler() = default;

    [[nodiscard]] constexpr HandlerID id() const { return handler_id; }

    // FIXME: Call operator
    template<typename E>
    void operator()(E const& e) {
        handler(e);
    }

    // private:
    static inline HandlerID id_counter = 0;

    HandlerID handler_id;
    std::type_index event_type_id;
    std::function<void(Event const&)> handler;
};

////////////////////////////////////////////////
// Event manager                //
////////////////////////////////////////////////

class EventManager {
public:
    EventManager() = delete;

    template<typename E>
        requires std::is_base_of_v<Event, E>
    static void send(E const& event) {
        E e = event;
        auto it = std::ranges::find_if(m_handlers, [](auto const& p) {
            return p.first == typeid(E);
        });

        if (it != m_handlers.end()) {
            for (auto const& event_handler : it->second) {
                event_handler.handler(e);
            }
        }
    }

    static void add_listener(EventHandler&& event_handler) {
        auto const event_type_id = event_handler.event_type_id;
        auto it =
            std::ranges::find_if(m_handlers, [event_type_id](auto const& p) {
                return p.first == event_type_id;
            });

        if (it != m_handlers.end()) {
            it->second.push_back(std::move(event_handler));
        }
        else {
            m_handlers.emplace_back(
                event_type_id,
                std::move(std::vector{ std::move(event_handler) })
            );
        }
    }

    static void remove_listener(
        std::type_index type_idx,
        EventHandler::HandlerID handler_id
    ) {
        auto it = std::ranges::find_if(m_handlers, [type_idx](auto const& p) {
            return p.first == type_idx;
        });

        auto rm_it = std::remove_if(
            it->second.begin(),
            it->second.end(),
            [handler_id](EventHandler& eh) { return handler_id == eh.id(); }
        );
        it->second.erase(rm_it, it->second.end());
    }

private:
    using EventToHandlers =
        std::pair<std::type_index, std::vector<EventHandler>>;
    static inline std::vector<EventToHandlers> m_handlers = {};
};

////////////////////////////////////////////////
// Event listener               //
////////////////////////////////////////////////

template<typename T, typename E>
// requires HasOnEvent<T, E> && IsEvent<E>
class EventListener {
public:
    EventListener() {
        auto handler = EventHandler(
            static_cast<T*>(this),
            static_cast<void (T::*)(E const&)>(&T::on_event)
        );
        m_handler_id = handler.id();
        EventManager::add_listener(std::move(handler));
    }

    ~EventListener() { EventManager::remove_listener(typeid(E), m_handler_id); }

    virtual void on_event(E const&) = 0;

private:
    EventHandler::HandlerID m_handler_id;
};

} // namespace peppe
