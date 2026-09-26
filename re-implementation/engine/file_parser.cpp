#include "file_parser.h"
#include <cassert>
#include <malloc.h>
#include <fstream>

/*
    This is a re-implementation of the "SStack" class from the original engine
    This will ingest any file you ask it to and provides utility functions for
    stepping through the chunks in the file. Some knowledge of the file structure
    of the specific file you are trying to read is necessary.
*/


file_parser::file_parser(char* file_name, bool allow_read, bool allow_write)
{
	_file_handle.open(file_name, std::ios_base::binary);

	if (!_file_handle.is_open())
	{
		printf("failed to open %s\n", file_name);
	}

    _cursor_position = 0;

    _file_handle.seekg(0, std::ios::end);
    _file_length = _file_handle.tellg();
    _file_handle.seekg(0, std::ios::beg);
    _allow_read = allow_read;
    _allow_write = allow_write;
    _end_of_chunk[0] = _file_length;
    _chunk_stack_level = 0;
    printf("Opened %s, length %u / %08X\n", file_name, _file_length, _file_length);
}


void file_parser::read_bytes_exactly(uint8_t* buffer, uint32_t length)
{
    if ((length + _cursor_position) > _file_length)
    {
        printf("invalid read of %u when only %u bytes exist!\n", length, _file_length - _cursor_position);
        exit(1);
    }
    if ((length + _cursor_position) > _end_of_chunk[_chunk_stack_level])
    {
        printf("invalid read of %u when only %u bytes exist in chunk!\n", length, _end_of_chunk[_chunk_stack_level] - _cursor_position);
        //exit(1);
    }
    if (buffer == nullptr)
    {
        _file_handle.seekg(length, std::ios::cur);
    }
    else
    {
        _file_handle.read(buffer, length);
    }
    _cursor_position += length;
	//printf("read %u, now cursor @ %04X\n", length, _cursor_position);
}


void file_parser::seek(uint32_t position, uint8_t offset)
{
    _file_handle.seekg(position, std::ios::beg);
}


bool file_parser::is_end_of_file(void)
{
    if (_cursor_position >= _file_length)
    {
        return true;
    }
    return false;
}


void file_parser::validate_header(void)
{
    if (_cursor_position != 0)
    {
        return;
    }

    uint32_t temp = 0;
    read_bytes_exactly((uint8_t*)&temp, 4);
    assert(temp == KIND_SR_HEADER);

    read_bytes_exactly((uint8_t*)&temp, 4);
    assert(temp == KIND_SR_HEADER_B);

    printf("4d:validated header\n");
}


uint8_t file_parser::read1(void)
{
    uint8_t out = 0;
    read_bytes_exactly((uint8_t*)&out, 1);
    return out;
}


uint32_t file_parser::read4(void)
{
    uint32_t out = 0;
    read_bytes_exactly((uint8_t*)&out, 4);
    return out;
}


uint16_t file_parser::read2(void)
{
    uint16_t out = 0;
    read_bytes_exactly((uint8_t*)&out, 2);
    return out;
}


cheetah_file_kind_t file_parser::read_kind(void)
{
    uint32_t out = 0;
    uint32_t length = 0;
    read_bytes_exactly((uint8_t*)&out, 4);
    read_bytes_exactly((uint8_t*)&length, 4);
	printf("%c%c%c%c: %u\n", ((char*)&out)[0], ((char*)&out)[1], ((char*)&out)[2], ((char*)&out)[3], length);
    if (_chunk_stack_level < (MAX_CHUNK_DEPTH - 1))
    {
        _chunk_stack_level++;
        uint32_t current_pos = _file_handle.tellg();
        _end_of_chunk[_chunk_stack_level] = current_pos + length;
        printf("> enter level %u expect %u\n", _chunk_stack_level, length);
        return (cheetah_file_kind_t)out;
    }
    else
    {
        printf("Max stack level of %u exceeded!\n");
        exit(1);
    }
}


float file_parser::read_float(void)
{
    float out = 0.0f;
    read_bytes_exactly((uint8_t*)&out, 4);
    return out;
}


uint16_t file_parser::read_string(char** buffer)
{
    uint16_t length = 0;
    read_bytes_exactly((uint8_t*)&length, 2);
    if (buffer == nullptr)
    {
        _file_handle.seekg(length, std::ios::cur);
        printf("readSting <skip nullptr>\n");
    }
    else
    {
        *buffer = (char*)malloc(length + 1);
        assert(*buffer != nullptr);
        (*buffer)[length] = 0;
        read_bytes_exactly(*buffer, length);
        printf("readString %s\n", *buffer);
    }
    return length;
}


bool file_parser::peek(void)
{
    if (_cursor_position >= _end_of_chunk[_chunk_stack_level])
    {
		printf("> end of section - %u >= %u\n", _cursor_position, _end_of_chunk[_chunk_stack_level]);
        return true;
    }
    else
    {
		printf("> not end of section\n");
        return false;
    }
}


void file_parser::pop(void)
{
	printf("> pop to level %u, set cursor to %u \n", _chunk_stack_level, _end_of_chunk[_chunk_stack_level]);
    seek(_end_of_chunk[_chunk_stack_level], SEEK_SET);
    _cursor_position = _end_of_chunk[_chunk_stack_level];
	_end_of_chunk[_chunk_stack_level] = 0;
    if (_chunk_stack_level > 0)
    {
        _chunk_stack_level--;
    }
	
}


void file_parser::print_tree(int indent)
{
    for (int i = 0; i < indent; i++)
    {
        printf(" ");
    }
    while (!peek())
    {
        printf("%c%c%c%c\n", read_kind());
        print_tree(indent + 1);
        pop();
    }
}
