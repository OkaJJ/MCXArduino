#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <thread>
#include <atomic>

#define PLUGGABLE_DISCOVERY_PROTOCOL 1
#define TARGET_VID 0x1fc9
#define TARGET_PID 0x015e

// State when we run the HELLO command.
bool idling = false;
// State when we run the START command.
bool running = false;
// State when we run the START_SYNC command.
std::atomic<bool> syncing = false;

std::thread sync_thread;

struct hid_device {
    std::string address;
    int vid;
    int pid;
    std::string phys_id;

    bool operator==(const hid_device& other) const {
        return address == other.address; 
    }
};

std::string int_to_hex(int i) {
    std::stringstream stream;
    stream << "0x" << std::setfill('0') << std::setw(4) << std::hex << i;
    return stream.str();
}

void print_json(const std::string& event_type, std::string content) {
    std::cout << "{\"eventType\":\"" + event_type + "\"," + content + "}" << std::endl;
}

std::string format_device_json(const hid_device& device) {
    return "{\"address\":\"" + device.address + 
           "\",\"label\":\"MCXArduino\",\"protocol\":\"blhost\",\"protocolLabel\":\"blhost over HID (USB)\",\"hardwareId\":\"" + device.phys_id + 
           "\",\"properties\":{\"pid\":\"" + int_to_hex(device.pid) + "\",\"vid\":\"" + int_to_hex(device.vid) + "\"}}";
}

std::vector<hid_device> hid_find_by_vidpid(int vid, int pid)
{
    namespace fs = std::filesystem;
    std::vector<hid_device> result;

    for (const auto& e : fs::directory_iterator("/sys/class/hidraw")) {
        const auto base = e.path().string();

        std::ifstream uevent(base + "/device/uevent");

        std::string address_s = e.path().filename();
        address_s.erase(remove(address_s.begin(), address_s.end(), '\"'), address_s.end());
        address_s = "/dev/" + address_s;

        if (!uevent) continue;

        std::string v;
        while(std::getline(uevent, v)) {
            // we found the correct line with HID info
            if(v.find("HID_ID") == 0) {
                std::string vid_s, pid_s;

                // find first and second colon
                auto c1 = v.find(":");
                auto c2 = v.find(":", c1 + 1);

                vid_s = v.substr(c1 + 1, c2 - c1 - 1);
                pid_s = v.substr(c2 + 1);

                // we compare with the required vid/pid and if its correct, then yay!
                if(std::stoi(vid_s, nullptr, 16) == vid && std::stoi(pid_s, nullptr, 16) == pid) {
                    // now we need atleast the HID_PHYS, ugly while in while i know :(
                    std::string phys_s;
                    while(std::getline(uevent, v)) {
                        if(v.find("HID_PHYS") == 0) {
                            phys_s = v.substr(v.find("=") + 1);
                            break;
                        }
                    }

                    result.push_back({address_s, vid, pid, phys_s});
                }
            }    
        }
    }
    return result;
}

void sync_routine() {
    std::vector<hid_device> last_devices;
    
    last_devices = hid_find_by_vidpid(TARGET_VID, TARGET_PID);
    
    for(const auto& dev : last_devices) {
        print_json("add", "\"port\":" + format_device_json(dev));
    }

    while(syncing) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        if (!syncing) break;

        auto current_devices = hid_find_by_vidpid(TARGET_VID, TARGET_PID);

        for(const auto& last : last_devices) {
            bool found = false;
            for(const auto& curr : current_devices) {
                if(last == curr) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                print_json("remove", "\"port\":" + format_device_json(last));
            }
        }

        for(const auto& curr : current_devices) {
            bool found = false;
            for(const auto& last : last_devices) {
                if(curr == last) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                print_json("add", "\"port\":" + format_device_json(curr));
            }
        }

        last_devices = current_devices;
    }
}

int main() {
    std::string cmd;
    while(std::getline(std::cin, cmd)) {
        
        // Hi hello, must be the first sent command, so we know our protocol versions and such and such.
        if(cmd.rfind("HELLO", 0) == 0) {
            std::string content_buffer = "\"protocolVersion\":1,\"message\":\"OK\"";

            idling = true;

            print_json("hello", content_buffer);
        }

        else if(cmd.rfind("START_SYNC", 0) == 0)  {
            std::string content_buffer = "\"message\":\"OK\"";

            idling = false;
            syncing = true;

            print_json("start_sync", content_buffer);
            sync_thread = std::thread(sync_routine);
        }

        // This command is used for initializing subroutines of the discovery program, but we dont really need anything, eh.
        else if(cmd.rfind("START", 0) == 0)  {
            std::string content_buffer = "\"message\":\"OK\"";

            idling = false;
            running = true;

            // Just sending ok always for now.
            print_json("start", content_buffer);
        }

        else if(cmd.rfind("STOP", 0) == 0) {
            std::string content_buffer = "\"message\":\"OK\"";

            if(syncing) {
                syncing = false;
                sync_thread.join();
            }
            if(running) {
                running = false;
            }

            print_json("stop", content_buffer);
        }

        else if(cmd.rfind("LIST", 0) == 0) {
            std::string content_buffer;

            auto devices = hid_find_by_vidpid(0x1fc9, 0x015e);
            
            content_buffer += "\"ports\":[";
            for(const auto& device : devices) {
                content_buffer += "{\"address\":\"" + device.address + "\",\"label\":\"MCXArduino\",\"protocol\":\"hid\",\"protocolLabel\":\"Human Interface Device (USB)\",\"hardwareId\":\"" + device.phys_id + "\",\"properties\":{\"pid\":\"" + int_to_hex(device.pid) + "\",\"vid\":\"" + int_to_hex(device.vid) + "\"}}";  
            }

            content_buffer += "]";

            print_json("list", content_buffer);
        }

        else if(cmd.rfind("QUIT", 0) == 0) {
            std::string content_buffer = "\"message\":\"OK\"";
            print_json("quit", content_buffer);
            exit(1);
        }
    }
}