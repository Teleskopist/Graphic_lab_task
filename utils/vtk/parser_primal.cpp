#include "vtk_internal.h"
#include "vtk_structures.h"
#include "parser.h"

#include <fstream>

namespace vtk
{
  BinaryBlock::BinaryBlock()
    {
      //don't want to deal with exceptions
      _data = (unsigned char*)malloc(START_BLOCK_SIZE);
      capacity = START_BLOCK_SIZE;
      if (!_data)
      {
        fprintf(stderr, "[BinaryBlock] failed to allocate initial memory\n");
        sz = 0;
        capacity = 0;
      }
    }
  
      BinaryBlock::BinaryBlock(const BinaryBlock &other)
    {
      _data = (unsigned char*)malloc(other.capacity);
      if (_data == nullptr)
      {
        fprintf(stderr, "[BinaryBlock] failed to allocate initial memory\n");
        sz = 0;
        capacity = 0;
      }
      else
      {
        capacity = other.capacity;
        sz = other.sz;
        memcpy(_data, other._data, sz);
      }
    }

    BinaryBlock::~BinaryBlock()
    {
      free(_data);
    }

    void BinaryBlock::add(unsigned char c)
    {
      if (sz == capacity)
        resize();

      _data[sz++] = c;
    }

    void BinaryBlock::resize()
    {
      uint64_t new_capacity = capacity < LINEAR_BLOCK_SIZE ? capacity * SIZE_MULT_FACTOR : capacity + LINEAR_BLOCK_SIZE;
      unsigned char *new_data = (unsigned char *)malloc(new_capacity);
      if (!new_data)
      {
        fprintf(stderr, "[BinaryBlock] failed to allocate memory (%d bytes)\n", (int)new_capacity);
        sz = 0;
        capacity = 0;
      }
      else
      {
        memcpy(new_data, _data, sz);
        free(_data);
        _data = new_data;
        capacity = new_capacity;
      }
    }

// base64_decode inplace, no allocation
void base64_decode_inplace(unsigned char *src, size_t len, size_t *out_len)
{
  const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  unsigned char dtable[256], *pos, block[4], tmp;
  size_t i, count;
  int pad = 0;

  memset(dtable, 0xFF, 256);
  for (i = 0; i < sizeof(base64_table) - 1; i++)
    dtable[base64_table[i]] = (unsigned char)i;
  dtable['='] = 0;

  pos = src;
  count = 0;
  for (i = 0; i < len; i++)
  {
    tmp = dtable[src[i]];
    if (tmp == 0xFF)
      continue;

    if (src[i] == '=')
      pad++;
    block[count] = tmp;
    count++;
    if (count == 4)
    {
      *pos++ = (block[0] << 2) | (block[1] >> 4);
      *pos++ = (block[1] << 4) | (block[2] >> 2);
      *pos++ = (block[2] << 6) | block[3];
      count = 0;
      if (pad)
      {
        if (pad == 1)
          pos--;
        else if (pad == 2)
          pos -= 2;
        else
        {
          printf("Invalid padding\n");
          return;
        }
        break;
      }
    }
  }

  *out_len = pos - src;
  return;
}

  enum PrimalParserState
  {
    READING_XML                  =  0,
    READING_COMMENT_OPEN_F       =  1, //!--
    READING_COMMENT_OPEN_L       =  2,
    READING_COMMENT_CLOSE_F      =  3, //-->
    READING_COMMENT_CLOSE_L      =  4,
    INSIDE_COMMENT               =  5,
    INSIDE_STRING_1              =  6, //'
    INSIDE_STRING_2              =  7, //"
    INSIDE_NODE                  =  8, //<
    READING_DATA_ARRAY_OPEN_F    =  9, //DataArray
    READING_DATA_ARRAY_OPEN_L    = 16,
    FINISHED_DATA_ARRAY_OPEN     = 17,
    READING_FORMAT_F             = 18,//format='
    READING_FORMAT_L             = 24,
    READING_FORMAT_NAME          = 25,
    READING_FORMAT_NAME_BINARY_F = 26,//binary'
    READING_FORMAT_NAME_BINARY_L = 31,
    INSIDE_DATA_ARRAY_BINARY     = 32,
    READING_FORMAT_NAME_A        = 33,//ascii' or appended'
    READING_FORMAT_NAME_ASCII_F  = 34,//scii'
    READING_FORMAT_NAME_ASCII_L  = 37,
    INSIDE_DATA_ARRAY_ASCII      = 38,
    BEFORE_BASE64_BINARY         = 39,
    INSIDE_BASE64_BINARY         = 40,
    BEFORE_ASCII_DATA            = 41,
    INSIDE_ASCII_DATA            = 42,
    INSIDE_DATA_ARRAY_NO_FORMAT  = 43,
    INSIDE_DATA_ARRAY_WHITESPACE = 44,
    READING_FORMAT_NAME_APPEND_F = 45,//ppended'
    READING_FORMAT_NAME_APPEND_L = 51,
    INSIDE_DATA_ARRAY_APPEND     = 52,
    READING_APPEND_DATA_OPEN_F   = 53, //AppendedData
    READING_APPEND_DATA_OPEN_L   = 63,
    BEFORE_APPEND_DATA           = 64,
    INSIDE_APPEND_DATA           = 65,
    READING_APPEND_DATA_CLOSE_F  = 66, //</AppendedData>
    READING_APPEND_DATA_CLOSE_L  = 79,

    RESTORE_STATE_BEFORE_STRING  = 80,
    START_STATE                  = 81,

    ERROR_STATE                  = 82, //generic error state
    ERROR_NO_FORMAT              = 83,
    ERROR_UNKNOWN_FORMAT         = 84,
    ERROR_EMPTY_BINARY_DATA      = 85,
    ERROR_EMPTY_ASCII_DATA       = 86,
    ERROR_EMPTY_APPEND_DATA      = 87,
    ERROR_TWO_APPEND_FILES       = 88,
    NUM_STATES
  };

  void fill_state_table(unsigned state_table[NUM_STATES][256])
  {
    constexpr unsigned CHAR_COUNT = 256;
    
    //reading xml if not met some useful characters
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[READING_XML][i] = READING_XML;
    state_table[READING_XML]['!'] = READING_COMMENT_OPEN_F;
    state_table[READING_XML]['<'] = INSIDE_NODE;
    state_table[READING_XML]['\''] = INSIDE_STRING_1;
    state_table[READING_XML]['"']  = INSIDE_STRING_2;


    //dealing with comments
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[READING_COMMENT_OPEN_F][i] = READING_XML;
    state_table[READING_COMMENT_OPEN_F]['-'] = READING_COMMENT_OPEN_L;

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[READING_COMMENT_OPEN_L][i] = READING_XML;  
    state_table[READING_COMMENT_OPEN_L]['-'] = INSIDE_COMMENT; 

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_COMMENT][i] = INSIDE_COMMENT;  
    state_table[INSIDE_COMMENT]['-'] = READING_COMMENT_CLOSE_F; 

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[READING_COMMENT_CLOSE_F][i] = INSIDE_COMMENT;
    state_table[READING_COMMENT_CLOSE_F]['-'] = READING_COMMENT_CLOSE_L;

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[READING_COMMENT_CLOSE_L][i] = INSIDE_COMMENT; 
    state_table[READING_COMMENT_CLOSE_L]['>'] = READING_XML;


    //dealing with strings
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_STRING_1][i] = INSIDE_STRING_1; 
    state_table[INSIDE_STRING_1]['\''] = RESTORE_STATE_BEFORE_STRING;

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_STRING_2][i] = INSIDE_STRING_2; 
    state_table[INSIDE_STRING_2]['"'] = RESTORE_STATE_BEFORE_STRING;

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_NODE][i] = READING_XML;
    state_table[INSIDE_NODE]['D'] = READING_DATA_ARRAY_OPEN_F;
    state_table[INSIDE_NODE]['A'] = READING_APPEND_DATA_OPEN_F;
    state_table[INSIDE_NODE]['"']  = INSIDE_STRING_2;
    state_table[INSIDE_NODE]['!'] = READING_COMMENT_OPEN_F;

    //data array open tag
    const char *data_array_name = "DataArray";
    for (int ch_i = 1; ch_i < strlen(data_array_name); ch_i++)
    {
      for (int i = 0; i < CHAR_COUNT; i++)
        state_table[READING_DATA_ARRAY_OPEN_F + ch_i - 1][i] = READING_XML;
      state_table[READING_DATA_ARRAY_OPEN_F + ch_i - 1][(unsigned char)data_array_name[ch_i]] = READING_DATA_ARRAY_OPEN_F + ch_i;
    }

    //appended data array open tag
    const char *appended_data_array_name = "AppendedData";
    for (int ch_i = 1; ch_i < strlen(appended_data_array_name); ch_i++)
    {
      for (int i = 0; i < CHAR_COUNT; i++)
        state_table[READING_APPEND_DATA_OPEN_F + ch_i - 1][i] = READING_XML;
      state_table[READING_APPEND_DATA_OPEN_F + ch_i - 1][(unsigned char)appended_data_array_name[ch_i]] = READING_APPEND_DATA_OPEN_F + ch_i;
    }

    //we are inside data array, but have not met the format keyword
    const char *whitespace_chars = " \t\n\r";
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_DATA_ARRAY_NO_FORMAT][i] = INSIDE_DATA_ARRAY_NO_FORMAT; 
    for (int i = 0; i < strlen(whitespace_chars); i++)
      state_table[INSIDE_DATA_ARRAY_NO_FORMAT][(unsigned char)whitespace_chars[i]] = INSIDE_DATA_ARRAY_WHITESPACE;
    state_table[INSIDE_DATA_ARRAY_NO_FORMAT]['>'] = ERROR_NO_FORMAT; //reached the end of the node and no format
    state_table[INSIDE_DATA_ARRAY_NO_FORMAT]['\''] = INSIDE_STRING_1;
    state_table[INSIDE_DATA_ARRAY_NO_FORMAT]['"']  = INSIDE_STRING_2;

    //reading format keyword
    const char *format_name  = "format=\'";
    const char *format_name2 = "format=\"";
    for (int ch_i = 1; ch_i < strlen(format_name); ch_i++)
    {
      for (int i = 0; i < CHAR_COUNT; i++)
        state_table[READING_FORMAT_F + ch_i - 1][i] = INSIDE_DATA_ARRAY_NO_FORMAT;
      for (int i = 0; i < strlen(whitespace_chars); i++)
        state_table[READING_FORMAT_F + ch_i - 1][(unsigned char)whitespace_chars[i]] = INSIDE_DATA_ARRAY_WHITESPACE;
      state_table[READING_FORMAT_F + ch_i - 1][(unsigned char)format_name [ch_i]] = READING_FORMAT_F + ch_i;
      state_table[READING_FORMAT_F + ch_i - 1][(unsigned char)format_name2[ch_i]] = READING_FORMAT_F + ch_i;
    }

    //reading format name
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[READING_FORMAT_NAME][i] = ERROR_UNKNOWN_FORMAT;
    state_table[READING_FORMAT_NAME]['a'] = READING_FORMAT_NAME_A;
    state_table[READING_FORMAT_NAME]['b'] = READING_FORMAT_NAME_BINARY_F;

    //starts with a, either ascii or appended
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[READING_FORMAT_NAME_A][i] = ERROR_UNKNOWN_FORMAT;
    state_table[READING_FORMAT_NAME_A]['s'] = READING_FORMAT_NAME_ASCII_F;
    state_table[READING_FORMAT_NAME_A]['p'] = READING_FORMAT_NAME_APPEND_F;

    //reading binary format name
    const char *binary_format_name  = "binary'";
    const char *binary_format_name2 = "binary\"";
    for (int ch_i = 1; ch_i < strlen(binary_format_name); ch_i++)
    {
      for (int i = 0; i < CHAR_COUNT; i++)
        state_table[READING_FORMAT_NAME_BINARY_F + ch_i - 1][i] = ERROR_UNKNOWN_FORMAT;
      state_table[READING_FORMAT_NAME_BINARY_F + ch_i - 1][(unsigned char)binary_format_name [ch_i]] = READING_FORMAT_NAME_BINARY_F + ch_i;
      state_table[READING_FORMAT_NAME_BINARY_F + ch_i - 1][(unsigned char)binary_format_name2[ch_i]] = READING_FORMAT_NAME_BINARY_F + ch_i;
    }

    //reading ascii format name
    const char *ascii_format_name  = "scii\'";
    const char *ascii_format_name2 = "scii\"";
    for (int ch_i = 1; ch_i < strlen(ascii_format_name); ch_i++)
    {
      for (int i = 0; i < CHAR_COUNT; i++)
        state_table[READING_FORMAT_NAME_ASCII_F + ch_i - 1][i] = ERROR_UNKNOWN_FORMAT;
      state_table[READING_FORMAT_NAME_ASCII_F + ch_i - 1][(unsigned char)ascii_format_name [ch_i]] = READING_FORMAT_NAME_ASCII_F + ch_i;
      state_table[READING_FORMAT_NAME_ASCII_F + ch_i - 1][(unsigned char)ascii_format_name2[ch_i]] = READING_FORMAT_NAME_ASCII_F + ch_i;
    }

    //reading appended format name
    const char *appended_format_name  = "ppended\'";
    const char *appended_format_name2 = "ppended\"";
    for (int ch_i = 1; ch_i < strlen(appended_format_name); ch_i++)
    {
      for (int i = 0; i < CHAR_COUNT; i++)
        state_table[READING_FORMAT_NAME_APPEND_F + ch_i - 1][i] = ERROR_UNKNOWN_FORMAT;
      state_table[READING_FORMAT_NAME_APPEND_F + ch_i - 1][(unsigned char)appended_format_name [ch_i]] = READING_FORMAT_NAME_APPEND_F + ch_i;
      state_table[READING_FORMAT_NAME_APPEND_F + ch_i - 1][(unsigned char)appended_format_name2[ch_i]] = READING_FORMAT_NAME_APPEND_F + ch_i;
    }

    //we are inside data array, expect base64 binary data at the end
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_DATA_ARRAY_BINARY][i] = INSIDE_DATA_ARRAY_BINARY; 
    state_table[INSIDE_DATA_ARRAY_BINARY]['>'] = BEFORE_BASE64_BINARY; //reached the end of the node, expect some whitespace and base64 data
    state_table[INSIDE_DATA_ARRAY_BINARY]['\''] = INSIDE_STRING_1;
    state_table[INSIDE_DATA_ARRAY_BINARY]['"']  = INSIDE_STRING_2;

    //we are inside data array, expect ascii data at the end
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_DATA_ARRAY_ASCII][i] = INSIDE_DATA_ARRAY_ASCII; 
    state_table[INSIDE_DATA_ARRAY_ASCII]['>'] = BEFORE_ASCII_DATA; //reached the end of the node, expect ascii data, deal with whitespaces later
    state_table[INSIDE_DATA_ARRAY_ASCII]['\''] = INSIDE_STRING_1;
    state_table[INSIDE_DATA_ARRAY_ASCII]['"']  = INSIDE_STRING_2;

    //we are inside data array, expect node to end without any data (it will be in the last node)
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_DATA_ARRAY_APPEND][i] = INSIDE_DATA_ARRAY_APPEND; 
    state_table[INSIDE_DATA_ARRAY_APPEND]['>'] = READING_XML; //reached the end of the node
    state_table[INSIDE_DATA_ARRAY_APPEND]['\''] = INSIDE_STRING_1;
    state_table[INSIDE_DATA_ARRAY_APPEND]['"']  = INSIDE_STRING_2;

    const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[BEFORE_BASE64_BINARY][i] = BEFORE_BASE64_BINARY;
    for (int i = 0; i < strlen(base64_chars); i++)
      state_table[BEFORE_BASE64_BINARY][(unsigned char)base64_chars[i]] = INSIDE_BASE64_BINARY;
    state_table[BEFORE_BASE64_BINARY]['<'] = ERROR_EMPTY_BINARY_DATA; //found next node, no base64 data

    const char *ascii_chars = " \t\n\r0123456789eE+-.uflL";
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[BEFORE_ASCII_DATA][i] = BEFORE_ASCII_DATA;
    for (int i = 0; i < strlen(ascii_chars); i++)
      state_table[BEFORE_ASCII_DATA][(unsigned char)ascii_chars[i]] = INSIDE_ASCII_DATA;
    state_table[BEFORE_ASCII_DATA]['<'] = ERROR_EMPTY_ASCII_DATA;

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_BASE64_BINARY][i] = READING_XML;
    for (int i = 0; i < strlen(base64_chars); i++)
      state_table[INSIDE_BASE64_BINARY][(unsigned char)base64_chars[i]] = INSIDE_BASE64_BINARY;
    
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_ASCII_DATA][i] = INSIDE_ASCII_DATA;
    state_table[INSIDE_ASCII_DATA]['<'] = READING_XML;

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[START_STATE][i] = READING_XML;

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[FINISHED_DATA_ARRAY_OPEN][i] = READING_XML;
    for (int i = 0; i < strlen(whitespace_chars); i++)
      state_table[FINISHED_DATA_ARRAY_OPEN][(unsigned char)whitespace_chars[i]] = INSIDE_DATA_ARRAY_WHITESPACE;
    
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_DATA_ARRAY_WHITESPACE][i] = INSIDE_DATA_ARRAY_NO_FORMAT;
    for (int i = 0; i < strlen(whitespace_chars); i++)
      state_table[INSIDE_DATA_ARRAY_WHITESPACE][(unsigned char)whitespace_chars[i]] = INSIDE_DATA_ARRAY_WHITESPACE;
    state_table[INSIDE_DATA_ARRAY_WHITESPACE]['f'] = READING_FORMAT_F;
    state_table[INSIDE_DATA_ARRAY_WHITESPACE]['\''] = INSIDE_STRING_1;
    state_table[INSIDE_DATA_ARRAY_WHITESPACE]['"']  = INSIDE_STRING_2;

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[BEFORE_APPEND_DATA][i] = BEFORE_APPEND_DATA;
    state_table[BEFORE_APPEND_DATA]['_'] = INSIDE_APPEND_DATA;
    state_table[BEFORE_APPEND_DATA]['<'] = ERROR_EMPTY_APPEND_DATA;

    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[INSIDE_APPEND_DATA][i] = INSIDE_APPEND_DATA;
    state_table[INSIDE_APPEND_DATA]['<'] = READING_APPEND_DATA_CLOSE_F;

    const char *append_data_close = "</AppendedData>";
    for (int ch_i = 1; ch_i < strlen(append_data_close); ch_i++)
    {
      for (int i = 0; i < CHAR_COUNT; i++)
        state_table[READING_APPEND_DATA_CLOSE_F + ch_i - 1][i] = INSIDE_APPEND_DATA;
      state_table[READING_APPEND_DATA_CLOSE_F + ch_i - 1][(unsigned char)append_data_close[ch_i]] = READING_APPEND_DATA_CLOSE_F + ch_i;
    }
    for (int i = 0; i < CHAR_COUNT; i++)
      state_table[READING_APPEND_DATA_CLOSE_L][i] = INSIDE_APPEND_DATA;
    state_table[READING_APPEND_DATA_CLOSE_L]['>'] = READING_XML;
  }

  bool primal_parse(const std::filesystem::path& filename, RawDataset &dataset)
  {
    bool full_debug = false;

    constexpr unsigned CHAR_COUNT = 256;
    unsigned state_table[NUM_STATES][CHAR_COUNT] = {ERROR_STATE};
    fill_state_table(state_table);
    
    unsigned char c;
    unsigned cur_state  = START_STATE;
    unsigned prev_state = START_STATE;
    unsigned state_before_string = START_STATE;
    //std::string binary_node_name = "<__binary_data />";

    auto t1 = std::chrono::high_resolution_clock::now();

    std::ifstream file(filename, std::ios::binary);
    while (cur_state < ERROR_STATE && !file.fail())
    {
      c = file.get();

      prev_state = cur_state;
      cur_state = state_table[cur_state][c];
      
      if (cur_state == RESTORE_STATE_BEFORE_STRING)
        cur_state = state_before_string;
      else if (cur_state != INSIDE_STRING_1 && cur_state != INSIDE_STRING_2)
        state_before_string = cur_state;

      if (cur_state == INSIDE_BASE64_BINARY)//create new binary file (base64)
      {
        if (prev_state != INSIDE_BASE64_BINARY)
        {
          dataset.binary_blocks.emplace_back();
          dataset.binary_block_formats.push_back(BinaryBlockFormat::BINARY_BASE64);
        }
      }
      else if (cur_state == INSIDE_ASCII_DATA)//create new binary file (ascii)
      {
        if (prev_state != INSIDE_ASCII_DATA)
        {
          dataset.binary_blocks.emplace_back();
          dataset.binary_block_formats.push_back(BinaryBlockFormat::ASCII);
        }
      }
      else if (cur_state == INSIDE_APPEND_DATA)//create new binary file (append)
      {
        if (prev_state == BEFORE_APPEND_DATA)
        {
          if (dataset.appended_data_block_id >= 0)
            cur_state = ERROR_TWO_APPEND_FILES;
          else
          {
            dataset.appended_data_block_id = dataset.binary_blocks.size(); 
            dataset.binary_blocks.emplace_back();
            dataset.binary_block_formats.push_back(BinaryBlockFormat::BINARY_RAW);
          }
        }
      }
      else if (prev_state == INSIDE_BASE64_BINARY) //finalize current binary file
      {
        char binary_node[256];
        snprintf(binary_node, 256, " <data type=\"binary_base64\" block_id=\"%zu\" bytesize=\"%d\"/>\n", 
                 dataset.binary_blocks.size() - 1, (int)dataset.binary_blocks.back().size()); 
        for (int i = 0; i < strlen(binary_node); i++)
          dataset.xml_data.add(binary_node[i]);
      }
      else if (prev_state == INSIDE_ASCII_DATA) //finalize current binary file
      {
        char binary_node[256];
        snprintf(binary_node, 256, " <data type=\"ascii\" block_id=\"%zu\" bytesize=\"%d\"/>\n", 
                 dataset.binary_blocks.size() - 1, (int)dataset.binary_blocks.back().size()); 
        for (int i = 0; i < strlen(binary_node); i++)
          dataset.xml_data.add(binary_node[i]);
        
        //make sure that the ASCII array is 0 terminated
        dataset.binary_blocks.back().add(0);
      }
      else if (prev_state == READING_APPEND_DATA_CLOSE_L) //finalize current binary file
      {
        char binary_node[256];
        snprintf(binary_node, 256, " <data type=\"append\" block_id=\"%zu\" bytesize=\"%d\"/>\n</AppendedData>\n", 
                 dataset.binary_blocks.size() - 1, (int)dataset.binary_blocks.back().size()); 
        for (int i = 0; i < strlen(binary_node); i++)
          dataset.xml_data.add(binary_node[i]);
      }

      if (cur_state == INSIDE_BASE64_BINARY || cur_state == INSIDE_ASCII_DATA || cur_state == INSIDE_APPEND_DATA ||
          prev_state == READING_APPEND_DATA_CLOSE_L || (cur_state >= READING_APPEND_DATA_CLOSE_F && cur_state <= READING_APPEND_DATA_CLOSE_L))
        dataset.binary_blocks.back().add(c);
      else if (c != (unsigned char)EOF)
        dataset.xml_data.add(c);
      
      if (full_debug)
        printf("%c(%d) %d -> %d\n", c, (unsigned)c, prev_state, cur_state);
    }

    if (cur_state >= ERROR_STATE)
    {
      switch (cur_state)
      {
      case ERROR_NO_FORMAT:
        fprintf(stderr, "[primal_parse::ERROR] no format specified for DataArray %d\n", (int)(dataset.binary_blocks.size()+1));
        break;

      case ERROR_UNKNOWN_FORMAT:
        fprintf(stderr, "[primal_parse::ERROR] unknown format of DataArray %d. Only ascii and binary (base64) are supported\n", (int)(dataset.binary_blocks.size()+1));
        break;
      
      case ERROR_EMPTY_BINARY_DATA:
        fprintf(stderr, "[primal_parse::ERROR] empty binary data for DataArray %d\n", (int)(dataset.binary_blocks.size()+1));
        break;
      
      case ERROR_EMPTY_ASCII_DATA:
        fprintf(stderr, "[primal_parse::ERROR] empty ascii data for DataArray %d\n", (int)(dataset.binary_blocks.size()+1));
        break;
      
      case ERROR_EMPTY_APPEND_DATA:
        fprintf(stderr, "[primal_parse::ERROR] empty append data for DataArray %d\n", (int)(dataset.binary_blocks.size()+1));
        break;
      
      case ERROR_TWO_APPEND_FILES:
        fprintf(stderr, "[primal_parse::ERROR] more than one append data file specified\n");
        break;
      
      case ERROR_STATE:
      default:
        fprintf(stderr, "[primal_parse::ERROR] illegal transition from %d with char <%c> (code=%d)\n", prev_state, c, (unsigned)c);
        break;
      }
      return false;
    }

    dataset.xml_data.add('\0');
    if (full_debug)
      printf("file = %s\n", dataset.xml_data.data());
    
    auto t2 = std::chrono::high_resolution_clock::now();
    
    //convert all base64 data to binary data
    for (int i = 0; i < dataset.binary_blocks.size(); i++)
    {
      if (dataset.binary_block_formats[i] == BinaryBlockFormat::BINARY_BASE64)
      {
        size_t prev_sz = dataset.binary_blocks[i].size();
        size_t out_len = 0;
        base64_decode_inplace(dataset.binary_blocks[i]._data, dataset.binary_blocks[i].size(), &out_len);
        dataset.binary_blocks[i].capacity = out_len;
        dataset.binary_blocks[i].sz = out_len;
        //printf("converted base64 block %d from %zu to %zu\n", i, prev_sz, out_len);
      }
    }

    auto t3 = std::chrono::high_resolution_clock::now();
    float time_parse_ms = 0.001f*std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
    float time_convert_ms = 0.001f*std::chrono::duration_cast<std::chrono::microseconds>(t3-t2).count();
    printf("parse took %.1f ms, convert took %.1f ms\n", time_parse_ms, time_convert_ms);
    
    return true;
  }

  bool  next_value(const char *p, char **end_ptr, uint8_t &val){ val = strtoul(p, end_ptr, 10); return p != *end_ptr;}
  bool  next_value(const char *p, char **end_ptr, uint16_t &val){ val = strtoul(p, end_ptr, 10); return p != *end_ptr;}
  bool  next_value(const char *p, char **end_ptr, uint32_t &val){ val = strtoul(p, end_ptr, 10); return p != *end_ptr;}
  bool  next_value(const char *p, char **end_ptr, uint64_t &val){ val = strtoul(p, end_ptr, 10); return p != *end_ptr;}
  bool  next_value(const char *p, char **end_ptr, int8_t &val){ val = strtol(p, end_ptr, 10); return p != *end_ptr;}
  bool  next_value(const char *p, char **end_ptr, int16_t &val){ val = strtol(p, end_ptr, 10); return p != *end_ptr;}
  bool  next_value(const char *p, char **end_ptr, int32_t &val){ val = strtol(p, end_ptr, 10); return p != *end_ptr;}
  bool  next_value(const char *p, char **end_ptr, int64_t &val){ val = strtol(p, end_ptr, 10); return p != *end_ptr;}
  bool  next_value(const char *p, char **end_ptr, float &val){ val = strtof(p, end_ptr); return p != *end_ptr;}
  bool  next_value(const char *p, char **end_ptr, double &val){ val = strtod(p, end_ptr); return p != *end_ptr;}

  template <typename T>
  bool read_ASCII_data(const BinaryBlock &block, std::vector<T> &out_values)
  {
    out_values.reserve(block.size()/sizeof(T));

    const char *p = (char *)block.data();
    while (p < (char *)block.data() + block.size())
    {
      while (isspace(*p))
        p++;

      if (*p == '\0')
        break;

      char *endPtr;
      T val;
      if (!next_value(p, &endPtr, val))
        return false;
      p = endPtr;
      out_values.push_back(val);
    }

    out_values.shrink_to_fit();
    return true;
  }

  bool read_ASCII_data(DataArray::Type type, const BinaryBlock &block, DataArray::Values &out_values)
  {
    out_values.type = type;
    switch (type)
    {
    case DataArray::Type::Int8:
    {
      std::vector<int8_t> values;
      if (!read_ASCII_data(block, values))
        return false;
      out_values.i8 = new int8_t[values.size()];
      memcpy(out_values.i8, values.data(), values.size()*sizeof(int8_t));
      out_values.count = values.size();
    }
    break;
    case DataArray::Type::UInt8:
    {
      std::vector<uint8_t> values;
      if (!read_ASCII_data(block, values))
        return false;
      out_values.u8 = new uint8_t[values.size()];
      memcpy(out_values.u8, values.data(), values.size()*sizeof(uint8_t));
      out_values.count = values.size();
    }
    break;
    case DataArray::Type::Int16:
    {
      std::vector<int16_t> values;
      if (!read_ASCII_data(block, values))
        return false;
      out_values.i16 = new int16_t[values.size()];
      memcpy(out_values.i16, values.data(), values.size()*sizeof(int16_t));
      out_values.count = values.size();
    }
    break;
    case DataArray::Type::UInt16:
    {
      std::vector<uint16_t> values;
      if (!read_ASCII_data(block, values))
        return false;
      out_values.u16 = new uint16_t[values.size()];
      memcpy(out_values.u16, values.data(), values.size()*sizeof(uint16_t));
      out_values.count = values.size();
    }
    break;
    case DataArray::Type::Int32:
    {
      std::vector<int32_t> values;
      if (!read_ASCII_data(block, values))
        return false;
      out_values.i32 = new int32_t[values.size()];
      memcpy(out_values.i32, values.data(), values.size()*sizeof(int32_t));
      out_values.count = values.size();
    }
    break;
    case DataArray::Type::UInt32:
    {
      std::vector<uint32_t> values;
      if (!read_ASCII_data(block, values))
        return false;
      out_values.u32 = new uint32_t[values.size()];
      memcpy(out_values.u32, values.data(), values.size()*sizeof(uint32_t));
      out_values.count = values.size();
    }
    break;
    case DataArray::Type::Int64:
    {
      std::vector<int64_t> values;
      if (!read_ASCII_data(block, values))
        return false;
      out_values.i64 = new int64_t[values.size()];
      memcpy(out_values.i64, values.data(), values.size()*sizeof(int64_t));
      out_values.count = values.size();
    }
    break;
    case DataArray::Type::UInt64:
    {
      std::vector<uint64_t> values;
      if (!read_ASCII_data(block, values))
        return false;
      out_values.u64 = new uint64_t[values.size()];
      memcpy(out_values.u64, values.data(), values.size()*sizeof(uint64_t));
      out_values.count = values.size();
    }
    break;
    case DataArray::Type::Float32:
    {
      std::vector<float> values;
      if (!read_ASCII_data(block, values))
        return false;
      out_values.f32 = new float[values.size()];
      memcpy(out_values.f32, values.data(), values.size()*sizeof(float));
      out_values.count = values.size();
    }
    break;
    case DataArray::Type::Float64:
    {
      std::vector<double> values;
      if (!read_ASCII_data(block, values))
        return false;
      out_values.f64 = new double[values.size()];
      memcpy(out_values.f64, values.data(), values.size()*sizeof(double));
      out_values.count = values.size();
    }
    break;
    default:
      return false;
    }
    return true;
  }
}
