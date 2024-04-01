# Todo

Backend:
- [x] Better serialization
- Implement all protocol messages and behaviour
    - [x] Set name
    - [ ] Peer discovery
- Refactor Code
    - [ ] Reevaluate the need for PeerTable and ConnectionTable types
- [ ] Fix most of string duplication
    - There are lots of std::string() calls
- [ ] Change ConnectionTable container (To vector map probably)
- [ ] Improve peppe::byte_reverse
- Security patch
    - [ ] Validation of input (socket)
    - [ ] Better error handling (more exceptions instead of just throwing
    ConnectionClosed)
    - [ ] Avoid race conditions.
    ConnectionTable is used simultaniously by multiple coroutines
    and the frontend thread
- [ ] Fix App crashing when exiting program

Frontend:
- [ ] Create logger that saves all logs in a container
- [ ] Implement UI for displaying logs
- [ ] Display connected peers (with name and IP)
- [ ] Pre process message before sending
- [ ] Highlight my messages

## When PeerDiscovery packet is received

- Create a list of all the addresses that I'm not already connected
- Connect to all of those peers

## Shared struct 

Create a data structure that stores all core shared
data between Backend and Frontend
```c++
struct ClientContext {
    std::string     client_name;
    ConnectionTable connection_table;
}
```

Propagates changes to client username
```c++
std::shared_ptr<ClientContext>
```