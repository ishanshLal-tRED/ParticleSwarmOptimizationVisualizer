#include <string>;
#include <eh.h>;

#include <logging.hxx>;

import MainApplication.Instancing;
typedef Instancing::Instance APP;

int main(size_t argc, char* argv[]) 
{
    // Be sure to enable "Yes with SEH Exceptions (/EHa)" in C++ / Code Generation;
_set_se_translator([](unsigned int u, _EXCEPTION_POINTERS *pExp) {
    std::string error = "SE Exception: ";
    switch (u) {
    case 0xC0000005:
        error += "Access Violation";
        break;
    default:
        char result[11];
        sprintf_s(result, 11, "0x%08X", u);
        error += result;
    };
    throw std::exception(error.c_str());
});

    new APP;

    try {
        APP::Setup (std::span<char*>{argv, argc});
    	APP::InitializeVk ();
    	APP::Run ();
    } catch (std::exception &e) {
        LOG_raw ("{:s}", e.what());
        return 1;
    }
    APP::TerminateVk ();
    APP::Cleanup ();
}
