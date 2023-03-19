#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <MemoryElement.hpp>
#include <LabelTable.hpp>

constexpr size_t STACK_SIZE{ 100 };

/**
 * @brief Class for the MIPS Simulator.
 */
class MIPSSimulator
{
    // Array to store names of registers
    std::string m_registers[32];
    // Array to store values of registers
    int32_t m_register_values[32];
    // Array to store names of instructions
    std::string m_instruction_set[17];
    // To store the Mode of execution
    int32_t m_mode;
    // To store the input program
    std::vector<std::string> m_input_program;
    // To store the number of lines in the program
    int32_t m_number_of_instructions;
    // To store the current instruction being worked with
    std::string m_current_instruction;
    // To store the line number being worked with
    int32_t m_program_counter;
    // To store the maximum length of the input program
    int32_t m_max_length;
    // Flag to check if program halted
    int32_t m_halt_value;
    // To store register names, values, etc. for the instruction
    int32_t r[3];
    // To store all the labels and addresses
    std::vector<LabelTable> m_table_of_labels;
    // To store all the memory elements
    std::vector<MemoryElement> m_memory;
    // Stack array
    int32_t m_stack[STACK_SIZE];

    void add();
    void addi();
    void sub();
    void mul();
    void andf();
    void andi();
    void orf();
    void ori();
    void nor();
    void slt();
    void slti();
    void lw();
    void sw();
    void beq();
    void bne();
    void j();
    /**
     * @brief Custom instruction for the simulator.
    */
    void halt();

    /**
     * @brief Store label names and addresses and memory names and values from
     *              the whole program.
    */
    void pre_process();

    /**
     * @brief Read an instruction, crop out comments and set the ProgramCounter
     *        value.
     *
     * @param line The line at which the instruction is located.
    */
    void read_instruction(int32_t line);

    /**
     * @brief Find out which instruction is to be executed and populate values
     *        in r[].
     *
     * @return The ID of the instruction that was just successfully parsed.
    */
    int32_t parse_instruction();

    /**
     * @brief Display line number and instruction at which error occurred and
     *        exit the program.
     */
    void report_error();

    /**
     * @brief Call the appropriate operation function based on the operation
     *        to execute.
     * @param instruction TODO
    */
    void execute_instruction(int32_t instruction);

    /**
     * @brief Check that between lower and upper-1 indices, str has only
     *        spaces, else report an error.
     *
     * @param lower TODO
     * @param upper 
     * @param str 
    */
    void only_spaces(int32_t lower, int32_t upper, std::string str);

    /**
     * @brief Remove spaces starting from first elements till they exist
     *        continuously in str.
     * @param str TOOD
    */
    void remove_spaces(std::string &str);

    /**
     * @brief Check that an stoi() on str would be valid, i.e., that str can be
     *        converted to an integer.
     *
     * @param str 
    */
    void assert_number(const std::string &str);

    /**
     * @brief Find which register has been specified and the populate the value
     *        in r[number].
     *
     * @param number 
    */
    void find_register(int32_t number);

    /**
     * @brief Find and return the label name.
     * @return 
    */
    std::string find_label();

    /**
     * @brief Check that first element is a ',' and to remove it.
    */
    void assert_remove_comma();

    /**
     * @brief Check that the offset for stack is valid and within bounds.
     * @param index 
    */
    void check_stack_bounds(int32_t index);

    /**
     * @brief Check that the label name does not start with a number and does
     *        not contain special characters.
     *
     * @param str
     */
    void assert_label_allowed(const std::string &str);

public:
    /**
     * @brief Create a new simulator for a basic MIPS processor.
     *
     * Initialize values for the simulator and pass the mode and the input
     * file path.
     *
     * @param mode TODO:
     * @param fileName The relative path to the .s-file with instructions.
    */
    MIPSSimulator(int32_t mode, const std::string &fileName);

    ~MIPSSimulator() = default;


    // This class is too large to copy around. Move it instead, if necessary.

    MIPSSimulator(const MIPSSimulator&) = delete;
    MIPSSimulator& operator=(const MIPSSimulator&) = delete;

    /**
     * @brief Run the simulator.
    */
    void execute();

    /**
     * @brief Print the current state of the internals of the CPU.
    */
    void display_state();
};


/**
 * @brief Returns true if the input char is an english letter or a number.
 * @param character 
 * @return 
*/
static constexpr bool is_ascii_alphanumerical(char character)
{
    return ('a' <= character && character <= 'z')
        || ('A' <= character && character <= 'Z')
        || ('0' <= character && character <= '9');
}
