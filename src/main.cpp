#include "hiddev.h"
#include "dsu.h"
#include "triton.h"
#include <cstdio>
#include <cstring>
#include <csignal>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>
#include <chrono>
#include <string>

using namespace std::chrono_literals;
using namespace sc2;

static std::atomic<bool> gShutdown{false};

static void signalHandler(int sig) {
    fprintf(stderr, "\nsc2gyrodsu: signal %d — shutting down\n", sig);
    gShutdown = true;
}

struct SlotReader {
    std::unique_ptr<HidSlot> slot;
    std::thread              thread;
    std::atomic<bool>        done{false};
};

static void slotReaderThread(HidSlot* slot, DsuServer* server, std::atomic<bool>& done) {
    const auto SILENCE_MAX = 2000ms;
    int      staleCnt  = 0;
    auto     lastSample = std::chrono::steady_clock::now();
    uint32_t lastTs    = 0;

    fprintf(stderr, "triton-%d: reader started (path=%s)\n",
            slot->dsuSlot, slot->path.c_str());

    while (!gShutdown && !done) {
        ControllerState state;
        bool ok = slot->readOne(state);

        if (ok) {
            lastSample = std::chrono::steady_clock::now();
            if (state.imu.timestamp_us == lastTs) {
                staleCnt++;
            } else {
                staleCnt = 0;
                lastTs   = state.imu.timestamp_us;
                server->pushState(state);
            }
        } else {
            if (std::chrono::steady_clock::now() - lastSample >= SILENCE_MAX) {
                fprintf(stderr, "triton-%d: no data for 2s — disconnecting\n", slot->dsuSlot);
                break;
            }
        }
    }

    fprintf(stderr, "triton-%d: reader exiting\n", slot->dsuSlot);
    server->setDisconnected(slot->dsuSlot);
    done = true;
}

static void printHelp(const char* name) {
    fprintf(stderr,
        "sc2gyrodsu — Cemuhook DSU server voor Steam Controller 2\n"
        "Gebruik: %s [--port PORT] [--expose] [--probe] [--help]\n", name);
}

int main(int argc, char* argv[]) {
    uint16_t port   = 26760;
    bool     expose = false;
    bool     probe  = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--probe") == 0)       probe  = true;
        else if (strcmp(argv[i], "--expose") == 0) expose = true;
        else if (strcmp(argv[i], "--help") == 0)   { printHelp(argv[0]); return 0; }
        else if (strcmp(argv[i], "--port") == 0 && i+1 < argc)
            port = (uint16_t)atoi(argv[++i]);
    }

    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

    HidManager manager;
    if (probe) { manager.probe(); return 0; }

    DsuServer server(port, expose);
    server.start();

    fprintf(stderr, "sc2gyrodsu draait. Wacht op emulator op 127.0.0.1:%u\n", port);

    bool slotUsed[4] = {};
    std::vector<std::unique_ptr<SlotReader>> readers;

    while (!gShutdown) {
        for (auto it = readers.begin(); it != readers.end(); ) {
            auto& r = **it;
            if (r.done) {
                slotUsed[r.slot->dsuSlot] = false;
                if (r.thread.joinable()) r.thread.join();
                it = readers.erase(it);
            } else ++it;
        }

        struct hid_device_info* devs = hid_enumerate(VID_VALVE, 0);
        struct hid_device_info* cur  = devs;
        while (cur && !gShutdown) {
            if (HidManager::isControllerInterface(*cur)) {
                bool already = false;
                for (auto& r : readers)
                    if (r->slot->path == cur->path) { already = true; break; }
                if (already) { cur = cur->next; continue; }

                int slot = -1;
                for (int i = 0; i < 4; ++i)
                    if (!slotUsed[i]) { slot = i; break; }
                if (slot < 0) { cur = cur->next; continue; }

                auto candidate = manager.openSlot(*cur, slot);
                if (!candidate) { cur = cur->next; continue; }

                fprintf(stderr, "scanner: PID %04X (%s) path=%s → slot %d\n",
                    cur->product_id, pidLabel(cur->product_id), cur->path, slot);

                slotUsed[slot] = true;
                auto reader = std::make_unique<SlotReader>();
                reader->slot = std::move(candidate);
                HidSlot* slotPtr = reader->slot.get();
                std::atomic<bool>& doneRef = reader->done;
                reader->thread = std::thread([slotPtr, &server, &doneRef](){
                    slotReaderThread(slotPtr, &server, const_cast<std::atomic<bool>&>(doneRef));
                });
                readers.push_back(std::move(reader));
            }
            cur = cur->next;
        }
        hid_free_enumeration(devs);
        std::this_thread::sleep_for(500ms);
    }

    server.stop();
    for (auto& r : readers) {
        r->done = true;
        if (r->thread.joinable()) r->thread.join();
    }
    return 0;
}
