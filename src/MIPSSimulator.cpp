#include <MIPSSimulator.hpp>

#include <iostream>
#include <fstream>
#include <algorithm>


MIPSSimulator::MIPSSimulator(
    int32_t mode,
    const std::string &file_name
)
    : m_max_length{ 10'000 }
    , m_number_of_instructions{}
    , m_program_counter{}
    , m_halt_value{}
{
    m_memory.clear();
    m_table_of_labels.clear();

    // Names of registers
    const std::string temp_registers[]{
        "zero", "at", "v0", "v1",
        "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8", "t9",
        "k0", "k1",
        "gp", "sp", "s8", "ra"
    };

    for (int32_t i{}; i < 32; i++)
        m_registers[i] = temp_registers[i];

    // Initialize registers to 0
    for (int32_t i{}; i < 32; i++)
        m_register_values[i] = 0;

    // Names of instructions allowed
    const std::string temp_instruction_set[]{
        "add", "sub",  "mul",
        "and", "or",   "nor",
        "slt", "addi", "andi",
        "ori", "slti", "lw",
        "sw",  "beq",  "bne",
        "j",   "halt"
    };

    for (int32_t i{}; i < 17; i++)
        m_instruction_set[i] = temp_instruction_set[i];

    // Initialize stack elements to 0
    for (int32_t i{}; i < STACK_SIZE; i++)
        m_stack[i] = 0;

    // Stack pointer at bottom element
    m_register_values[29] =      40'396;
    m_register_values[28] = 100'000'000;
    // Set mode
    m_mode = mode;

    std::ifstream input_file{};
    input_file.open(file_name.c_str(), std::ios::in);

    // If open failed
    if (!input_file)
    {
        std::cout << "Error: File does not exist or could not be opened.\n";
        exit(1);
    }

    std::string temp_string;
    // Read line by line
    while (getline(input_file, temp_string))
    {
        m_number_of_instructions++;
        // Check number of instructions with maximum allowed
        if (m_number_of_instructions>m_max_length)
        {
            std::cout << "Error: Number of lines in input too large, maximum "
                "allowed is " << m_max_length << " lines.\n";
            exit(1);
        }

        // Store in m_input_program.
        m_input_program.push_back(temp_string);
    }

    input_file.close();
}


void MIPSSimulator::execute()
{
    // To remove effect of pressing enter key while starting
    getchar();
    // Populate list of memory elements and labels
    pre_process();
    // Traverse instructions till end or till halt
    while (m_program_counter < m_number_of_instructions && m_halt_value == 0)
    {
        read_instruction(m_program_counter);
        remove_spaces(m_current_instruction);

        // Ignore blank instructions
        if (m_current_instruction == "")
        {
            m_program_counter++;
            continue;
        }

        // Get operationID
        int32_t instruction{ parse_instruction() };
        execute_instruction(instruction);

        // If not jump, update ProgramCounter here
        if (instruction < 13 || instruction > 15)
            m_program_counter++;

        // If step by step mode, display state and wait
        if (m_mode == 0 && m_halt_value == 0)
        {
            display_state();
            getchar();
        }
    }
    // Display state at end.
    display_state();
    // If program ended without halt
    if (m_halt_value == 0)
    {
        std::cout << "Error: Program ended without halt.\n";
        exit(1);
    }

    std::cout << "\nExecution completed successfully.\n\n";
}


void MIPSSimulator::pre_process()
{
    int32_t i;
    int32_t j;

    // current_section == 0 -> data section
    // current_section == 1 -> text section
    int32_t current_section{ -1 };
    // To hold index of ".data"
    int32_t index;
    int32_t comment_index;
    // Whether "..data" found
    int32_t flag{};
    std::string temp_string{};
    int	is_label{};
    int	done_flag{};
    // Line number for start of data section
    int32_t data_start{};
    int32_t text_start{};

    for (i = 0; i < m_number_of_instructions; i++)
    {
        read_instruction(i);
        if (m_current_instruction.empty())
            continue;

        // Search for beginning of data section
        index = m_current_instruction.find(".data");

        // Not found
        if (index == -1)
            continue;
        // Found .data
        else if (flag == 0)
        {
            // Set as found
            flag = 1;
            // Assert that nothing is before
            only_spaces(0, index, m_current_instruction);
            // Assert that nothing is after
            only_spaces(
                index + 5,
                m_current_instruction.size(),
                m_current_instruction
            );

            // Set section
            current_section = 0;
            // Set location where section starts
            data_start = i;
        }
        // For multiple findings of ".data"
        else if (flag == 1)
        {
            std::cout << "Error: Multiple instances of .data.\n";
            report_error();
        }
    }

    // To store index of ":"
    int32_t label_index;

    // In data section
    if (current_section == 0)
    {
        for (i = data_start + 1; i < m_number_of_instructions; i++)
        {
            read_instruction(i);
            remove_spaces(m_current_instruction);

            if (m_current_instruction.empty())
                continue;

            label_index = m_current_instruction.find(":");
            // If ":" not found
            if (label_index == -1)
            {
                // f text section has not started
                if (m_current_instruction.find(".text") == -1)
                {
                    std::cout << "Error: Unexpected symbol in data section.\n";
                    report_error();
                } else
                    break;
            }

            // If found at first place
            if (label_index == 0)
            {
                std::cout << "Error: Label name expected.\n";
                report_error();
            }

            // Ignore spaces before ":" till label
            j = label_index - 1;
            while (
                j >= 0
                &&
                m_current_instruction[j] ==' '
                || 
                m_current_instruction[j] =='\t'
            )
                j--;

            //To store label name
            temp_string = "";
            // Label name complete
            int32_t done_flag{};
            for (j = 0; j >= 0; j--)
            {
                // Till label name characters are being found
                if (
                    m_current_instruction[j] !=' '
                    &&
                    m_current_instruction[j] != '\t'
                    &&
                    done_flag == 0
                )
                    temp_string = m_current_instruction[j]+temp_string;
                // If something found after name ends
                else if (
                    m_current_instruction[j] != ' '
                    &&
                    m_current_instruction[j] != '\t'
                    &&
                    done_flag == 1
                )
                {
                    std::cout << "Error: Unexpected text before label name.\n";
                    report_error();
                }
                // Name ended
                else
                    done_flag = 1;
            }

            // Check validity of name
            assert_label_allowed(temp_string);

            // Create memory and store memory element
            MemoryElement temp_memory{ .label = temp_string };

            // Search for ".word" in the same way
            uint64_t word_index{ m_current_instruction.find(".word") };
            if (word_index == -1)
            {
                std::cout << "Error: .word not found.\n";
                report_error();
            }

            only_spaces(label_index + 1, word_index, m_current_instruction);

            int32_t found_value{};
            int32_t done_finding{};
            temp_string = "";
            for (j = word_index + 5; j < m_current_instruction.size(); j++)
            {
                if (
                    found_value == 1
                    &&
                    (
                        m_current_instruction[j] == ' '
                        ||
                        m_current_instruction[j] == '\t'
                    )
                    &&
                    done_finding == 0
                )
                    done_finding = 1;

                else if (
                    found_value == 1
                    &&
                    m_current_instruction[j] != ' '
                    &&
                    m_current_instruction[j] != '\t'
                    &&
                    done_finding == 1
                )
                {
                    std::cout << "Error: Unexpected text after value.\n";
                    report_error();
                } else if (
                    found_value == 0
                    &&
                    m_current_instruction[j] != ' '
                    &&
                    m_current_instruction[j] != '\t'
                )
                {
                    found_value = 1;
                    temp_string = temp_string + m_current_instruction[j];
                } else if (
                    found_value == 1
                    &&
                    m_current_instruction[j] != ' '
                    &&
                    m_current_instruction[j] != '\t'
                )
                    temp_string = temp_string + m_current_instruction[j];
            }

            // Check that number found is a valid integer
            assert_number(temp_string);
            // Change type and store
            temp_memory.value = stoi(temp_string);
            // Add to list
            m_memory.push_back(temp_memory);
        }
    }

    // Sort for easy comparison
    sort(m_memory.begin(), m_memory.end(), MemoryElement::sort_memory);

    // Check for duplicates
    for (i = 0; m_memory.size() > 0 && i < m_memory.size() - 1; i++)
    {
        if (m_memory[i].label == m_memory[i + 1].label)
        {
            std::cout << "Error: One or more labels are repeated.\n";
            exit(1);
        }
    }

    int32_t text_flag{};
    int32_t text_index{};
    
    for (i = m_program_counter; i < m_number_of_instructions; i++)
    {
        read_instruction(i);
        if (m_current_instruction.empty())
            continue;

        // Find text section similar as above
        text_index = m_current_instruction.find(".text");
        if (text_index == -1)
            continue;
        else if (text_flag == 0)
        {
            text_flag = 1;
            only_spaces(0, text_index, m_current_instruction);
            only_spaces(
                text_index + 5,
                m_current_instruction.size(),
                m_current_instruction
            );
            current_section = 1;
            text_start = i;
        } else if (text_flag == 1)
        {
            std::cout << "Error: Multiple instances of .text.\n";
            report_error();
        }
    }

    // If text section not found
    if (current_section != 1)
    {
        std::cout << "Error: Text section does not exist or found unknown string.\n";
        exit(1);
    }

    // Location of main label
    int32_t main_index{};
    // Whether main label found
    int32_t found_main{};
    label_index = 0;
    for (i = text_start + 1; i < m_number_of_instructions; i++)
    {
        read_instruction(i);
        if (m_current_instruction.empty())
            continue;

        // Find labels similar as above
        label_index = m_current_instruction.find(":");
        if (label_index == 0)
        {
            std::cout << "Error: Label name expected.\n";
            report_error();
        }

        if (label_index == -1)
            continue;

        j = label_index - 1;
        while (
            j >= 0 && m_current_instruction[j] == ' '
            ||
            m_current_instruction[j] == '\t'
        )
            j--;

        temp_string = "";
        is_label    = 0;
        done_flag   = 0;

        for (; j >= 0; j--)
        {
            if (
                m_current_instruction[j] != ' '
                &&
                m_current_instruction[j] != '\t'
                &&
                done_flag == 0
            )
            {
                is_label    = 1;
                temp_string = m_current_instruction[j] + temp_string;
            } else if (
                m_current_instruction[j] != ' '
                &&
                m_current_instruction[j] != '\t'
                &&
                done_flag == 1
            )
            {
                std::cout << "Error: Unexpected text before label name.\n";
                report_error();
            } else if (is_label == 0)
            {
                std::cout << "Error: Label name expected.\n";
                report_error();
            } else
                done_flag = 1;
        }

        assert_label_allowed(temp_string);
        // Check that nothing is after label
        only_spaces(
            label_index + 1,
            m_current_instruction.size(),
            m_current_instruction
        );

        // For main, set variables as needed
        if (temp_string == "main")
        {
            found_main = 1;
            main_index = m_program_counter + 1;
        } else
        {
            LabelTable temp_label_table{
                .label   = temp_string,
                .address = m_program_counter
            };

            // Store labels
            m_table_of_labels.push_back(temp_label_table);
        }
    }

    // Sort labels
    sort(
        m_table_of_labels.begin(),
        m_table_of_labels.end(),
        LabelTable::sort_table
    );

    // Check for duplicates
    for (
        i = 0;
        m_table_of_labels.size() > 0 && i < m_table_of_labels.size() - 1;
        i++
    )
    {
        if (m_table_of_labels[i].label==m_table_of_labels[i + 1].label)
        {
            std::cout << "Error: One or more labels are repeated.\n";
            exit(1);
        }
    }

    // If main label not found
    if (found_main == 0)
    {
        std::cout << "Error: Could not find main.\n";
        exit(1);
    }

    // Set program counter.
    m_program_counter = main_index;

    std::cout << "Initialized and ready to execute. ";
    std::cout << "Current state is as follows : \n";
    display_state();
    std::cout <<"\nStarting execution\n\n";
}


void MIPSSimulator::report_error()
{
    const int32_t line_number{ m_program_counter + 1 };
    const std::string instruction_line{ m_input_program[m_program_counter] };

    std::cout
        << "Error found in line: " << line_number
        << ": " << instruction_line << '\n';

    display_state();
    exit(1);
}


void MIPSSimulator::read_instruction(int32_t line)
{
    // Set current_instruction
    m_current_instruction = m_input_program[line];
    // Remove comments
    if (m_current_instruction.find("#") != -1)
    {
        const auto new_instr{
            m_current_instruction.substr(
                0, m_current_instruction.find('#')
            )
        };

        m_current_instruction = new_instr;
    }

    // Set m_program_counter
    m_program_counter = line;
}


int32_t MIPSSimulator::parse_instruction()
{
    // Trim line.
    remove_spaces(m_current_instruction);

    // If label encountered
    if (m_current_instruction.find(":") != -1)
        return -2;

    // No valid instruction is this small
    if (m_current_instruction.size() < 4)
    {
        std::cout << "Error: Unknown operation.\n";
        report_error();
    }

    int32_t j;
    // Find length of operation
    for (j = 0; j < 4; j++)
        if (m_current_instruction[j] == ' ' || m_current_instruction[j] == '\t')
            break;

    // Cut the operation out
    std::string operation{ m_current_instruction.substr(0, j) };
    if (
        m_current_instruction.size() > 0
        &&
        j < m_current_instruction.size() - 1
    )
        m_current_instruction = m_current_instruction.substr(j + 1);

    int32_t found_op{};         // Whether valid operation or not
    int32_t operation_ID{ -1 }; // Set operation index from array

    int32_t i;
    // Check operation with allowed operations
    for (i = 0; i < 17; i++)
        if (operation == m_instruction_set[i])
        {
            operation_ID = i;
            break;
        }

    // If not valid
    if (operation_ID == -1)
    {
        std::cout << "Error: Unknown operation.\n";
        report_error();
    }

    // For R-format instructions
    if (operation_ID < 7)
    {
        // Find three registers separated by commas and put them in r[]
        for (int32_t count{}; count <3 ; count++)
        {
            remove_spaces(m_current_instruction);
            find_register(count);
            remove_spaces(m_current_instruction);

            if (count == 2)
                break;

            assert_remove_comma();
        }

        // If something more found
        if (!m_current_instruction.empty())
        {
            std::cout << "Error: Extra arguments provided.\n";
            report_error();
        }
    }
    // For I-format instructions
    else if (operation_ID < 11)
    {
        // Find two registers separated by commas
        for (int32_t count{}; count < 2; count++)
        {
            remove_spaces(m_current_instruction);
            find_register(count);
            remove_spaces(m_current_instruction);
            assert_remove_comma();
        }

        remove_spaces(m_current_instruction);
        // Find third argument, a number
        std::string temp_string{ find_label() };
        // Check validity
        assert_number(temp_string);
        // Convert and store
        r[2] = stoi(temp_string);
    }
    // For lw, sw
    else if (operation_ID < 13)
    {
        std::string temp_string{};
        int32_t offset;

        remove_spaces(m_current_instruction);
        // Find source/destination register
        find_register(0);
        remove_spaces(m_current_instruction);
        // Find comma, ignoring extra spaces
        assert_remove_comma();
        remove_spaces(m_current_instruction);

        // If offset type
        if (
            (
                m_current_instruction[0] > 47
                &&
                m_current_instruction[0] < 58
            )
            ||
            m_current_instruction[0] == '-'
        )
        {
            j = 0;
            while (
                j < m_current_instruction.size()
                &&
                m_current_instruction[j] != ' '
                &&
                m_current_instruction[j] != '\t'
                &&
                m_current_instruction[j] != '('
            )
            {
                // Find offset
                temp_string = temp_string+m_current_instruction[j];
                j++;
            }

            // If instruction ends there
            if (j == m_current_instruction.size())
            {
                std::cout << "Error: '(' expected.\n";
                report_error();
            }

            // Check validity of offset
            assert_number(temp_string);
            // Convert and store
            offset = stoi(temp_string);
            m_current_instruction = m_current_instruction.substr(j);
            remove_spaces(m_current_instruction);

            if (
                m_current_instruction.empty()
                ||
                m_current_instruction[0] != '('
                ||
                m_current_instruction.size() < 2
            )
            {
                std::cout << "Error: '(' expected.\n";
                report_error();
            }

            m_current_instruction = m_current_instruction.substr(1);
            remove_spaces(m_current_instruction);
            // Find register containing address
            find_register(1);
            remove_spaces(m_current_instruction);

            if (
                m_current_instruction.empty()
                ||
                m_current_instruction[0] != ')'
            )
            {
                std::cout << "Error: ')' expected.\n";
                report_error();
            }

            m_current_instruction = m_current_instruction.substr(1);
            only_spaces(0, m_current_instruction.size(), m_current_instruction);
            r[2] = offset;

            // -1 reserved for non offset type, anyway an invalid offset,
            // others checked later
            if (r[2] == -1)
            {
                std::cout << "Error: Invalid offset.\n";
                report_error();
            }

        }
        // If label type
        else
        {
            // Find label
            temp_string = find_label();
            int32_t found_location;

            for (j = 0; j < m_memory.size(); j++)
            {
                // Check for label from memory
                if (temp_string == m_memory[j].label)
                {
                    // Label found
                    found_location = 1;
                    if (operation_ID == 11)
                        // For lw, send value
                        r[1] = m_memory[j].value;
                    else
                        // For sw, send index in memory
                        r[1] = j;
                    break;
                }
            }

            // If label not found
            if (found_location == 0)
            {
                std::cout << "Error: Invalid label.\n";
                report_error();
            }

            // To indicate that it is not offset type
            r[2] = -1;
        }
    } 
    // For beq, bne
    else if (operation_ID < 15)
    {
        // Find two registers separated by commas
        for (int32_t count{}; count < 2; count++)
        {
            remove_spaces(m_current_instruction);
            find_register(count);
            remove_spaces(m_current_instruction);
            assert_remove_comma();
        }

        remove_spaces(m_current_instruction);
        // Find label
        std::string tempString = find_label();
        int32_t foundAddress{};

        // Search for label
        for (j = 0; j < m_table_of_labels.size(); j++)
        {
            // If label found
            if (tempString == m_table_of_labels[j].label)
            {
                foundAddress = 1;
                // Set r[2]
                r[2] = m_table_of_labels[j].address;
                break;
            }
        }

        // If label not found
        if (foundAddress == 0)
        {
            std::cout << "Error: Invalid label.\n";
            report_error();
        }
    }
    // For j
    else if (operation_ID == 15)
    {
        remove_spaces(m_current_instruction);
        int32_t foundAddress{};
        // Find jump label
        std::string tempString = find_label();
        // Search for label
        for (j = 0; j < m_table_of_labels.size(); j++)
        {
            if (tempString == m_table_of_labels[j].label)
            {
                // If label found
                foundAddress = 1;
                // Set r[0]
                r[0] = m_table_of_labels[j].address;
            }
        }

        // If label not found
        if (foundAddress == 0)
        {
            std::cout << "Error: Invalid label.\n";
            report_error();
        }
    }
    // For halt.
    else if (operation_ID == 16)
        remove_spaces(m_current_instruction);

    return operation_ID;
}


void MIPSSimulator::only_spaces(
    int32_t lower,
    int32_t upper,
    std::string str
)
{
    for (int32_t i{ lower }; i < upper; i++)
    {
        // Check that only ' ' and '\t' characters exist
        if (str[i] != ' ' && str[i] != '\t')
        {
            std::cout << "Error: Unexpected character.\n";
            report_error();
        }
    }
}


void MIPSSimulator::execute_instruction(int32_t instruction)
{
    // Call appropriate function based on the value of instruction
    switch (instruction)
    {
    case  0: add();  break;
    case  1: sub();  break;
    case  2: mul();  break;
    case  3: andf(); break;
    case  4: orf();  break;
    case  5: nor();  break;
    case  6: slt();  break;
    case  7: addi(); break;
    case  8: andi(); break;
    case  9: ori();  break;
    case 10: slti(); break;
    case 11: lw();   break;
    case 12: sw();   break;
    case 13: beq();  break;
    case 14: bne();  break;
    case 15: j();    break;
    case 16: halt(); break;

    case -2:
        // If instruction containing label, ignore
        break;
    default:
        std::cout << "Error: Invalid instruction received.\n";
        report_error();
    }
}


void MIPSSimulator::remove_spaces(std::string &str)
{
    int32_t j{};

    // Till only ' ' or '\t' found
    while (
        j < str.size()
        &&
        (
            str[j] == ' '
            ||
            str[j] == '\t'
        )
    ) j++;

    // Remove all of those
    str = str.substr(j);
}


void MIPSSimulator::add()
{
    // Check that value of stack pointer is within bounds
    if (r[0] == 29)
        check_stack_bounds(m_register_values[r[1]] + m_register_values[r[2]]);

    // Cannot modify $zero or use $at
    if (r[0] != 0 && r[0] != 1 && r[1] != 1 && r[2] != 1)
    {
        // Execute
        m_register_values[r[0]] =
            m_register_values[r[1]] + m_register_values[r[2]];
    } else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::addi()
{
    if (r[0] == 29)
        check_stack_bounds(m_register_values[r[1]] + r[2]);

    if (r[0] != 0 && r[0] != 1 && r[1] != 1)
    {
        m_register_values[r[0]] = m_register_values[r[1]] + r[2];
    } else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::sub()
{
    if (r[0] == 29)
        check_stack_bounds(m_register_values[r[1]]-m_register_values[r[2]]);

    if (r[0] != 0 && r[0] != 1 && r[1] != 1 && r[2] != 1)
        m_register_values[r[0]] =
            m_register_values[r[1]] - m_register_values[r[2]];
    else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::mul() // last 32 bits?
{
    if (r[0] == 29)
        check_stack_bounds(m_register_values[r[1]] * m_register_values[r[2]]);

    if (r[0] != 0 && r[0] != 1 && r[1] != 1 && r[2] != 1)
        m_register_values[r[0]] =
            m_register_values[r[1]] * m_register_values[r[2]];
    else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::andf()
{
    if (r[0] == 29)
        check_stack_bounds(m_register_values[r[1]] & m_register_values[r[2]]);

    if (r[0] != 0 && r[0] != 1 && r[1] != 1 && r[2] != 1)
        m_register_values[r[0]] =
            m_register_values[r[1]] & m_register_values[r[2]];
    else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::andi()
{
    if (r[0] == 29)
    {
        check_stack_bounds(m_register_values[r[1]]&r[2]);
    }
    if (r[0] != 0 && r[0] != 1 && r[1] != 1)
    {
        m_register_values[r[0]] = m_register_values[r[1]]&r[2];
    } else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::orf()
{
    if (r[0] == 29)
        check_stack_bounds(m_register_values[r[1]] | m_register_values[r[2]]);

    if (r[0] != 0 && r[0] != 1 && r[1] != 1 && r[2] != 1)
        m_register_values[r[0]] =
            m_register_values[r[1]] | m_register_values[r[2]];
    else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::ori()
{
    if (r[0] == 29)
        check_stack_bounds(m_register_values[r[1]] | r[2]);

    if (r[0] != 0 && r[0] != 1 && r[1] != 1)
        m_register_values[r[0]] = m_register_values[r[1]] | r[2];
    else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::nor()
{
    if (r[0] == 29)
        check_stack_bounds(
            ~(m_register_values[r[1]] | m_register_values[r[2]])
        );
    if (r[0] != 0 && r[0] != 1 && r[1] != 1 && r[2] != 1)
        m_register_values[r[0]] =
            ~(m_register_values[r[1]] | m_register_values[r[2]]);
    else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::slt()
{
    if (r[0] != 0 && r[0] != 1 && r[1] != 1 && r[2] != 1)
        m_register_values[r[0]] =
            m_register_values[r[1]] < m_register_values[r[2]];
    else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::slti()
{
    if (r[0] != 0 && r[0] != 1 && r[1] != 1)
        m_register_values[r[0]] = m_register_values[r[1]] < r[2];
    else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::lw()
{
    if (r[0] == 29)
        check_stack_bounds(r[1]);

    // If label type
    if (r[0] != 0 && r[0] != 1 && r[2]==-1)
        m_register_values[r[0]] = r[1];
    // if offset type
    else if (r[0] != 0 && r[0] != 1)
    {
        // check validity of offset
        check_stack_bounds(m_register_values[r[1]]+r[2]);
        m_register_values[r[0]] =
            m_stack[(m_register_values[r[1]] + r[2] - 40000) / 4];
    } else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::sw()
{
    // If label type
    if (r[0] != 1 && r[2]==-1)
        m_memory[r[1]].value = m_register_values[r[0]];
    // If offset type
    else if (r[0] != 1)
    {
        // Check validity of offset
        check_stack_bounds(m_register_values[r[1]] + r[2]);
        m_stack[(m_register_values[r[1]] + r[2] - 40000) / 4] =
            m_register_values[r[0]];
    } else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::beq()
{
    if (r[0] != 1 && r[1] != 1)
    {
        if (m_register_values[r[0]] == m_register_values[r[1]])
            // If branch taken, update ProgramCounter with new address
            m_program_counter = r[2]; 
        else
            // Else increment as usual
            m_program_counter++;
    } else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::bne()
{
    if (r[0] != 1 && r[1] != 1)
    {
        if (m_register_values[r[0]] != m_register_values[r[1]])
            m_program_counter = r[2];
        else
            m_program_counter++;
    } else
    {
        std::cout << "Error: Invalid usage of registers.\n";
        report_error();
    }
}


void MIPSSimulator::j()
{
    // Update ProgramCounter to address
    m_program_counter = r[0];
}


void MIPSSimulator::halt()
{
    // Set halt value to halt the program
    m_halt_value = 1;
}


void MIPSSimulator::display_state()
{
    // starting address of memory
    int32_t current_address{ 40'000 };

    // Display current instruction
    if (m_program_counter < m_number_of_instructions)
        std::cout
            << "\nExecuting instruction: "
            << m_input_program[m_program_counter]
            << '\n';
    else
        // To display at the end, where
        // m_program_counter == m_number_of_instructions and is out of bounds
        std::cout
            << "\nExecuting instruction: "
            << m_input_program[m_program_counter - 1]
            << '\n';

    // Display ProgramCounter
    std::cout << "\nProgram Counter: " << (4 * m_program_counter) << "\n\n";
    std::cout << "Registers:\n\n";

    printf(
        "%11s%12s\t\t%10s%12s\n",
        "Register", "Value", "Register", "Value"
    );

    // Display registers
    for (int32_t i{}; i < 16; i++)
        printf(
            "%6s[%2d]:%12d\t\t%5s[%2d]:%12d\n", 
            m_registers[i].c_str(),
            i,
            m_register_values[i],
            m_registers[i + 16].c_str(),
            i + 16,
            m_register_values[i+16]
        );

    // Display memory
    std::cout << "\nMemory:.\n";
    std::cout << "Address    Label   Value      Address    Label   Value    Address    Label   Value     Address    Label   Value     Address    Label   Value    .\n";
    // Stack
    for (int32_t i{}; i < 20; i++)
        printf(
            "%7x%8s:%8d\t%5x%8s:%8d\t%9x%8s:%8d\t%6x%8s:%8d\t%11x%8s:%8d\n",

            current_address + 4 * i,
            "<Stack>",
            m_stack[i],
            current_address + 4 * (i + 20),
            "<Stack>",
            m_stack[i + 20],
            current_address + 4 * (i + 40),
            "<Stack>",
            m_stack[i + 40],
            current_address + 4 * (i + 60),
            "<Stack>",
            m_stack[i + 60],
            current_address + 4 * (i + 80),
            "<Stack>",
            m_stack[i + 80]
        );

    current_address += 400;
    // Labels
    for (int32_t i{}; i < m_memory.size(); i++)
        printf(
            "%7x%8s:%8d\n",

            40'400 + 4 * i,
            m_memory[i].label.c_str(),
            m_memory[i].value
        );

    std::cout << '\n';
}


void MIPSSimulator::assert_number(const std::string &str)
{
    // Check that each character is a digit.
    for (int32_t j{}; j < str.size(); j++)
    {
        // Ignore minus sign
        if (j == 0 && str[j] == '-')
            continue;

        if (str[j] < 48 || str[j] > 57)
        {
            std::cout << "Error: Specified value is not a number.\n";
            report_error();
        }
    }

    // Check against maximum allowed for 32 bit integer
    if (
        str[0] != '-'
        &&
        (
            str.size() > 10
            ||
            (
                str.size() == 10
                &&
                str > "2147483647"
            )
        )
    )
    {
        std::cout << "Error: Number out of range.\n";
        report_error();
    }
    // Same check as above for negative integers
    else if (
        str[0] == '-'
        &&
        (
            str.size() > 11
            ||
            (
                str.size() == 11
                &&
                str > "-2147483648"
            )
        )
    )
    {
        std::cout << "Error: Number out of range.\n";
        report_error();
    }
}


void MIPSSimulator::find_register(int32_t number)
{
    int32_t found_register{};
    // Find '$' sign
    if (
        m_current_instruction[0] != '$'
        ||
        m_current_instruction.size() < 2
    )
    {
        std::cout << "Error: Register expected.\n";
        report_error();
    }

    // Remove '$' sign
    m_current_instruction = m_current_instruction.substr(1);
    // Next two characters to match
    std::string register_ID{ m_current_instruction.substr(0, 2) };

    // For $zero, need four characters
    if (register_ID == "ze" && m_current_instruction.size() >= 4)
        register_ID += m_current_instruction.substr(2, 2);
    else if (register_ID == "ze")
    {
        std::cout << "Error: Register expected.\n";
        report_error();
    }

    for (int32_t i{}; i < 32; i++)
    {
        // Find register from list
        if (register_ID == m_registers[i])
        {
            // Populate r[number]
            r[number] = i;
            // Set flag as found
            found_register = 1;
            // Remove name based on whether $zero or something else
            if (i != 0)
                m_current_instruction = m_current_instruction.substr(2);
            else
                m_current_instruction = m_current_instruction.substr(4);
        }
    }

    // If register not found
    if (found_register == 0)
    {
        std::cout << "Error: Invalid register.\n";
        report_error();
    }
}


std::string MIPSSimulator::find_label()
{
    // Remove spaces
    remove_spaces(m_current_instruction);
    // To store label
    std::string temp_string{};
    // Found
    int32_t found_value{};
    // Completed finding
    int32_t done_finding{};
    for (int32_t j{}; j < m_current_instruction.size(); j++)
    {
        if (
            found_value == 1
            &&
            (
                m_current_instruction[j] == ' '
                ||
                m_current_instruction[j] == '\t'
            )
            &&
            done_finding == 0
        )
            // When space encountered after start, label finding done
            done_finding = 1;
        else if (
            found_value == 1
            &&
            m_current_instruction[j] != ' '
            &&
            m_current_instruction[j] != '\t'
            &&
            done_finding == 1
        )
        {
            // If non space encountered after done, some incorrect character
            // found
            std::cout << "Error: Unexpected text after value.\n";
            report_error();
        } else if (
            found_value == 0
            &&
            m_current_instruction[j] != ' '
            &&
            m_current_instruction[j] != '\t'
        )
        {
            // When first non space found, finding starts
            found_value = 1;
            temp_string = temp_string + m_current_instruction[j];
        } else if (
            found_value == 1
            &&
            m_current_instruction[j] != ' '
            &&
            m_current_instruction[j] != '\t'
        )
            // Continue finding
            temp_string = temp_string + m_current_instruction[j];
    }

    // Return found label
    return temp_string;
}


void MIPSSimulator::assert_remove_comma()
{
    // Check that first element exists and is a comma
    if (
        m_current_instruction.size() < 2
        ||
        m_current_instruction[0] != ','
    )
    {
        std::cout << "Error: Comma expected.\n";
        report_error();
    }

    // Remove it
    m_current_instruction = m_current_instruction.substr(1);
}


void MIPSSimulator::check_stack_bounds(int32_t index)
{
    // Check that address is within stack bounds and a multiple of 4
    if (
        !(
            index <= 40'396
            &&
            index >= 40'000
            &&
            index % 4 == 0
        )
    )
    {
        std::cout << "Error: Invalid address for stack pointer. "
            "To access data section, use labels instead of addresses.\n";
        report_error();
    }
}


void MIPSSimulator::assert_label_allowed(const std::string &str)
{
    //  Check that label size is at least one and the first value is not
    //  an integer.
    if (str.size() == 0 || ('0' <= str[0] && str[0] <= '9'))
    {
        std::cout << "Error: Invalid label: Label begins with a number.\n";
        report_error();
    }

    for (int32_t i = 0; i < str.size(); i++)
    {
        //  Check that only numbers and letters are used.
        if (!is_ascii_alphanumerical(str[i]))
        {
            std::cout << "Error: Invalid label.\n";
            report_error();
        }
    }
}
