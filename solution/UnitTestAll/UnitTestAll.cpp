#include <filesystem>
#include <regex>
#include <cstdlib> // std::system
#include <iostream>
#include "Windows.h"

// Run all unit tests in current directory

namespace fs = std::experimental::filesystem;
using namespace std::regex_constants;

int main( ) {
    const auto this_filename{ fs::path{ _pgmptr }.filename( ).string( ) };
    const std::regex unitTestRegex{ R"(.*unittest.*\.exe)", icase };

    for( const auto& dir : fs::directory_iterator{ fs::current_path( ) } ) {
        const auto path = dir.path( );
        if( fs::is_regular_file( path ) ) {

            const auto file_name = path.filename( );
            
            if( file_name.string( ) != this_filename &&
                std::regex_match( file_name.string( ), unitTestRegex ) ) 
            {
                std::cout << "Start test: " << file_name << '\n';
                std::system( file_name.string( ).c_str( ) );
                std::cout << "Finsh test: " << file_name << "\n\n";
            }
        }
    }
}

