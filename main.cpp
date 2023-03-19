//  STL Import
#include <iostream>

//  Project Import
#include <LabelTable.hpp>
#include <MemoryElement.hpp>
#include <MIPSSimulator.hpp>


int main()
{
    std::string path;
    int32_t mode;

    std::cout << "\nMIPS Simulator\n\n";

    std::cout <<
        "Program to simulate execution in MIPS Assembly language!\n"
        "Two modes are available:\n\n"

        "1. Step by Step Mode - View state after each instruction\n"
        "2. Execution Mode - View state after end of execution\n\n";

    std::cout << "Enter the relative path of the input file and the mode number:\n";

    std::cin >> path >> mode;
    //  If mode is invalid
    if (mode != 1 && mode != 2)
    {
        std::cout << "Error: Invalid Mode.\nExiting...\n";
        return 1;
    }

    //  Create and initialize simulator
    MIPSSimulator simulator{ mode - 1, path };
    //  Execute simulator
    simulator.execute();

    auto _ignore{ std::getchar() };
}
