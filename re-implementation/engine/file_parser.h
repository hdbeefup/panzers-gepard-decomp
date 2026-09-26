#ifndef __FILE_PARSER_H__
#define __FILE_PARSER_H__

#include "defines.h"
#include <fstream>

#define MAX_CHUNK_DEPTH     32

class file_parser
{
    public:
        file_parser(char* file_name, bool allow_read, bool allow_write);
        void validate_header(void);
        bool is_end_of_file(void);
        void read_bytes_exactly(uint8_t* buffer, uint32_t length);
        void read_up_to_bytes(uint8_t* buffer, uint32_t length);
        void write_bytes(uint8_t* buffer, uint32_t length);
        uint16_t read_string(char** buffer);
        void seek(uint32_t position, uint8_t offset);
        bool peek(void);
        void pop(void);
        uint32_t read4(void);
        uint16_t read2(void);
        uint8_t read1(void);
        cheetah_file_kind_t read_kind(void);
        float    read_float(void);

        void print_tree(int indent);

    private:
        std::ifstream _file_handle;
        uint32_t _cursor_position;
        uint32_t _file_length;
        bool _allow_read;
        bool _allow_write;
        uint32_t _end_of_chunk[MAX_CHUNK_DEPTH];
        uint8_t _chunk_stack_level;
};

#endif
