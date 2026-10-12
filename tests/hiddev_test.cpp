#include "hiddev.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct hid_device_ {
    bool active;
};

static int closed = 0;
static std::array<unsigned char, 54> report{};
static int reportLength = report.size();

extern "C" {
int hid_init() { return 0; }
int hid_exit() { return 0; }
hid_device_info* hid_enumerate(unsigned short, unsigned short) { return nullptr; }
void hid_free_enumeration(hid_device_info*) {}
hid_device* hid_open_path(const char* path) {
    if (strcmp(path, "/denied") == 0) return nullptr;
    return new hid_device_{strcmp(path, "/empty") != 0};
}
void hid_close(hid_device* dev) { ++closed; delete dev; }
const wchar_t* hid_error(hid_device*) { return L"access denied"; }
int hid_set_nonblocking(hid_device*, int) { return 0; }
int hid_send_feature_report(hid_device*, const unsigned char*, size_t length) {
    return static_cast<int>(length);
}
int hid_read_timeout(hid_device* dev, unsigned char* data, size_t length, int) {
    if (!dev->active) return 0;
    if (length < static_cast<size_t>(reportLength)) return -1;
    memcpy(data, report.data(), reportLength);
    return reportLength;
}
}

static void require(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}

int main() {
    using namespace sc2;
    HidManager manager;
    hid_device_info info{};
    info.product_id = PID_PROTEUS_PUCK;
    info.usage_page = 0xFF00;
    info.usage = 1;
    wchar_t serial[] = L"SHARED-PUCK-SERIAL";
    info.serial_number = serial;
    require(HidManager::isControllerInterface(info), "puck channel must be eligible");
    info.usage = 2;
    require(!HidManager::isControllerInterface(info), "puck control interface must be excluded");
    info.product_id = PID_NEREID_PUCK;
    require(!HidManager::isControllerInterface(info), "Nereid control interface must be excluded");
    info.usage = 1;
    require(HidManager::isControllerInterface(info), "Nereid channel must be eligible");
    for (auto pid : {PID_TRITON_WIRED, PID_TRITON_BLE}) {
        info.product_id = pid;
        require(HidManager::isControllerInterface(info), "direct connections must remain eligible");
    }
    info.usage_page = 1;
    require(!HidManager::isControllerInterface(info), "mouse/keyboard collections must be excluded");
    info.usage_page = 0xFF00;
    info.product_id = 0x1102;
    require(!HidManager::isControllerInterface(info), "unsupported controller must be excluded");
    info.product_id = PID_PROTEUS_PUCK;

    char empty[] = "/empty";
    info.path = empty;
    require(!manager.openSlot(info, 0), "empty channel must not reserve a slot");
    require(closed == 1, "empty channel handle must be closed");
    char denied[] = "/denied";
    info.path = denied;
    require(!manager.openSlot(info, 0), "inaccessible channel must not reserve a slot");

    report[0] = REPORT_ID_STATE;
    report[1 + IMU_OFFSET] = 1;
    char first[] = "/channel-1";
    info.path = first;
    auto one = manager.openSlot(info, 0);
    require(one && one->dsuSlot == 0 && one->path == first,
            "first active channel must use slot zero after empty channels");
    char second[] = "/channel-2";
    info.path = second;
    auto two = manager.openSlot(info, 1);
    require(two && two->path != one->path,
            "channels sharing a receiver serial must retain distinct identities");
    ControllerState state;
    require(one->readOne(state) && state.slot == 0 && state.imu.timestamp_us == 1,
            "state decoding must be preserved");
    report[0] = REPORT_ID_STATE_BLE;
    require(one->readOne(state), "BLE state decoding must be preserved");
    report[0] = 0x40;
    require(!manager.openSlot(info, 2), "mouse reports must not reserve a slot");
    report[0] = REPORT_ID_STATE;
    reportLength = 8;
    require(!manager.openSlot(info, 2), "truncated reports must not reserve a slot");
    two.reset();
    one.reset();
    require(closed == 5, "all opened HID handles must be closed");
}
