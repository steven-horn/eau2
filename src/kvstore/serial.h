// lang::CwC
#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "object.h"
#include "string.h"
#include "array.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

/**
  * An object that knows how to convert other objects into character arrays
  * and vice versa. Specific implementations such as MessageSerializer
  * also exist.
  * @author horn.s@husky.neu.edu, armani.a@husky.neu.edu
  */
class Serializer: public Object {
public:

  // Serializes an unsigned int.
  const char* serialize(size_t sz) {
      ByteArray* barr = new ByteArray();
      char siz[16];
      sprintf(siz, "%zu", sz);
      barr->push_string(siz);
      const char* str = barr->as_bytes();
      delete barr;
      return str;
  }

  // Serializes a double.
  const char* serialize(double d) {
      ByteArray* barr = new ByteArray();
      char dbl[16];
      sprintf(dbl, "%f", d);
      barr->push_string(dbl);
      const char* str = barr->as_bytes();
      delete barr;
      return str;
  }

  // Serializes an int.
  const char* serialize(int i) {
      ByteArray* barr = new ByteArray();
      char num[16];
      sprintf(num, "%i", i);
      barr->push_string(num);
      const char* str = barr->as_bytes();
      delete barr;
      return str;
  }

  // Serializes a networking address. (port, family, ip address)
  const char* serialize(struct sockaddr_in addr) {
      ByteArray* barr = new ByteArray();

      char buff[100];
      memset(buff, 0, 100);

      char fam[4];            // buff for sin_family (ip4, ip6, non)
      memset(fam, 0, 4);
      if (addr.sin_family == AF_INET) memcpy(fam, "ip4", 3);
      else if (addr.sin_family == AF_INET6) memcpy(fam, "ip6", 3);
      else if (addr.sin_family == AF_UNSPEC) memcpy(fam, "non", 3);

      char adr[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(addr.sin_addr), adr, INET_ADDRSTRLEN);

      // addr: sin_family, sin_port, sin_addr.s_addr
      sprintf(buff, "\tfam: %s\n\tprt: %hu\n\tadr: %s",
              fam, addr.sin_port, adr);
      barr->push_string(buff);

      const char* str = barr->as_bytes();
      delete barr;
      return str;
  }

  // Deserializes a networking struct.
  struct sockaddr_in get_sockaddr_in(const char* str, size_t* i) {
      size_t port;
      const char* adr;
      size_t n = 5;
      char fam[4];
      memset(fam, 0, 4);

      // go through lines of str
      while (n < strlen(str)) {
          ++n;
          // get the type of this line
          char type_buff[4];
          memcpy(type_buff, &str[n], 3);
          type_buff[3] = 0;
          // this is a family line
          if (strcmp(type_buff, "fam") == 0) {
              n += 5;
              memcpy(fam, &str[n], 3);
              n += 4;
          }
          // this is a prt line
          else if (strcmp(type_buff, "prt") == 0) port = get_size(&str[n], &n);
          // this is a adr line
          else if (strcmp(type_buff, "adr") == 0) adr = get_adr(&str[n], &n);
          else break;
      }

      struct sockaddr_in addr;
      if (strcmp(fam, "ip4") == 0) addr.sin_family = AF_INET;
      else if (strcmp(fam, "ip6") == 0) addr.sin_family = AF_INET6;
      else if (strcmp(fam, "non") == 0) addr.sin_family = AF_UNSPEC;

      inet_pton(AF_INET, adr, &addr.sin_addr);

      addr.sin_port = port;

      (*i) += (n - 1);

      return addr;
  }

  // Deserializes a size_t.
  size_t get_size(const char* str, size_t* i) {
      size_t new_line_loc, sz;
      size_t n = 5;
      // find the end of the line
      for (size_t j = n; str[j] != '\n' && str[j] != 0; ++j) {
          new_line_loc = j;
      }
      ++new_line_loc;

      // get the sender idx
      char buff[new_line_loc - n + 1];
      memcpy(buff, &str[n], new_line_loc - n);
      buff[new_line_loc - n] = 0;
      sz = atoi(buff);

      (*i) += new_line_loc + 1;

      return sz;
  }

  // Deserializes an integer.
  int get_int(const char* str, size_t* i) {
      return (int)get_size(str, i);
  }

  // Deserializes an address.
  const char* get_adr(const char* str, size_t* i) {
      size_t new_line_loc;
      size_t n = 5;
      const char* adr;
      // find the end of the line
      for (size_t j = n; str[j] != '\n' && str[j] != 0; ++j) {
          new_line_loc = j;
      }
      ++new_line_loc;

      // get the sender idx
      char buff[new_line_loc - n + 1];
      memcpy(buff, &str[n], new_line_loc - n);
      buff[new_line_loc - n] = 0;
      adr = buff;

      (*i) += new_line_loc + 1;

      return adr;
  }

  // Converts string object into char array.
  const char* serialize(String* str) {
      ByteArray* barr = new ByteArray();

      // wrap string literal in quotes
      barr->push_back('\"');
      barr->push_string(str->c_str());
      barr->push_back('\"');

      const char* str_ = barr->as_bytes();
      delete barr;
      return str_;
  }

  // Deserializes a string.
  String* get_string(const char* str) {
      return new String(str);
  }

  // Converts an array of String objects into its char* equivalent.
  const char* serialize(StringArray* sarr) {
      ByteArray* barr = new ByteArray();

      // serialize the size
      barr->push_string("siz: ");
      const char* ser_sz = serialize(sarr->size());
      barr->push_string(ser_sz);
      delete[] ser_sz;

      // serialize the array
      barr->push_string("\narr: ");
      for (size_t i = 0; i < sarr->size() - 1; ++i) {
          const char* ser_str = serialize(sarr->get(i));
          barr->push_string(ser_str);
          barr->push_string(", ");
          delete[] ser_str;
      }

      // add last element of array
      const char* ser_str = serialize(sarr->get(sarr->size() - 1));
      barr->push_string(ser_str);
      delete[] ser_str;

      const char* str = barr->as_bytes();
      delete barr;
      return str;
  }

  // Deserializes a String Array.
  StringArray* get_string_array(const char* str) {
      size_t arr_size, new_line_loc, i;

      StringArray* sarr = new StringArray();

      // go through lines of str
      i = 0;
      while (i < strlen(str)) {
          // get the type of this line
          char type_buff[4];
          memcpy(type_buff, &str[i], 3);
          type_buff[3] = 0;
          // this is a sz line
          if (strcmp(type_buff, "siz") == 0) {
              i += 4;
              // find the end of the line
              for (size_t j = i; str[j] != '\n' && str[j] != 0; ++j) {
                  new_line_loc = j;
              }
              ++new_line_loc;
              // get the size
              char sz_buff[new_line_loc - i + 1];
              memcpy(sz_buff, &str[i], new_line_loc - i);
              sz_buff[new_line_loc - i] = 0;
              arr_size = atoi(sz_buff);
              i = new_line_loc + 1;
          // this is an ar line
          } else if (strcmp(type_buff, "arr") == 0) {
              // get the string array line
              char* ar_line = duplicate(&str[i + 5]);
              i += 5;
              // split into tokens and add to sarr
              char* string;
              string = strtok(ar_line, "\", ");
              while (string != NULL) {
                  String* new_str = new String(string);
                  sarr->push_back(new_str);
                  string = strtok(NULL, "\", ");
              }
              // find the end of the line
              for (size_t j = i; str[j] != '\n' && str[j] != 0; ++j) {
                  new_line_loc = j;
              }
              i = new_line_loc + 1;
              delete[] ar_line;
          }
      }

      // make sure sizes match up
      assert(arr_size == sarr->size());

      return sarr;
  }

  // Converts a Double array into its char* equivalent.
  const char* serialize(DoubleArray* darr) {
      ByteArray* barr = new ByteArray();

      // serialize size
      barr->push_string("siz: ");
      const char* ser_sz = serialize(darr->size());
      barr->push_string(ser_sz);
      delete[] ser_sz;

      // serialize array
      barr->push_string("\narr: ");
      for (size_t i = 0; i < darr->size() - 1; ++i) {
          const char* ser_dbl = serialize(darr->get(i));
          barr->push_string(ser_dbl);
          barr->push_string(", ");
          delete[] ser_dbl;
      }

      // add last element of array
      const char* ser_dbl = serialize(darr->get(darr->size() - 1));
      barr->push_string(ser_dbl);
      delete[] ser_dbl;

      const char* str = barr->as_bytes();
      delete barr;
      return str;
  }

  // Deserializes a double array.
  DoubleArray* get_double_array(const char* str) {
      size_t arr_size, new_line_loc, i;

      DoubleArray* darr = new DoubleArray();

      // go through lines of str
      i = 0;
      while (i < strlen(str)) {
          //printf("%zu, %zu\n", i, strlen(str));
          // get the type of this line
          char type_buff[4];
          memcpy(type_buff, &str[i], 3);
          type_buff[3] = 0;
          // this is a sz line
          if (strcmp(type_buff, "siz") == 0) {
              i += 5;
              // find the end of the line
              for (size_t j = i; str[j] != '\n' && str[j] != 0; ++j) {
                  new_line_loc = j;
              }
              ++new_line_loc;
              // get the size
              char sz_buff[new_line_loc - i + 1];
              memcpy(sz_buff, &str[i], new_line_loc - i);
              sz_buff[new_line_loc - i] = 0;
              arr_size = atoi(sz_buff);
              i = new_line_loc + 1;
          // this is an ar line
          } else if (strcmp(type_buff, "arr") == 0) {
              // get the double array line
              char* ar_line = duplicate(&str[i + 5]);
              i += 5;
              // split into tokens and add to darr
              char* next_double;
              next_double = strtok(ar_line, ", ");
              while (next_double != NULL) {
                  i += strlen(next_double) + 2;
                  darr->push_back(atof(next_double));
                  next_double = strtok(NULL, ", ");
              }
              delete[] ar_line;
          }
      }

      // make sure sizes match up
      assert(arr_size == darr->size());

      return darr;
  }

  // Converts a Bool array into its char* equivalent.
  const char* serialize(BoolArray* arr) {
      ByteArray* barr = new ByteArray();

      // serialize size
      barr->push_string("siz: ");
      const char* ser_sz = serialize(arr->size());
      barr->push_string(ser_sz);
      delete[] ser_sz;

      // serialize array
      barr->push_string("\narr: ");
      for (size_t i = 0; i < arr->size() - 1; ++i) {
          if (arr->get(i)) barr->push_string("true");
          else barr->push_string("false");
          barr->push_string(", ");
      }

      // add last element of array
      if (arr->get(arr->size() - 1)) barr->push_string("true");
      else barr->push_string("false");

      const char* str = barr->as_bytes();
      delete barr;
      return str;
  }

  // Deserializes a bool array.
  BoolArray* get_bool_array(const char* str) {
      size_t arr_size, new_line_loc, i;

      BoolArray* barr = new BoolArray();

      // go through lines of str
      i = 0;
      while (i < strlen(str)) {
          //printf("%zu, %zu\n", i, strlen(str));
          // get the type of this line
          char type_buff[4];
          memcpy(type_buff, &str[i], 3);
          type_buff[3] = 0;
          // this is a sz line
          if (strcmp(type_buff, "siz") == 0) {
              i += 5;
              // find the end of the line
              for (size_t j = i; str[j] != '\n' && str[j] != 0; ++j) {
                  new_line_loc = j;
              }
              ++new_line_loc;
              // get the size
              char sz_buff[new_line_loc - i + 1];
              memcpy(sz_buff, &str[i], new_line_loc - i);
              sz_buff[new_line_loc - i] = 0;
              arr_size = atoi(sz_buff);
              i = new_line_loc + 1;
          // this is an ar line
          } else if (strcmp(type_buff, "arr") == 0) {
              // get the double array line
              char* ar_line = duplicate(&str[i + 5]);
              i += 5;
              // split into tokens and add to darr
              char* next_bool;
              next_bool = strtok(ar_line, ", ");
              while (next_bool != NULL) {
                  i += strlen(next_bool) + 2;
                  if (strcmp(next_bool, "true") == 0) barr->push_back(1);
                  else barr->push_back(0);
                  next_bool = strtok(NULL, ", ");
              }
              delete[] ar_line;
          }
      }

      // make sure sizes match up
      assert(arr_size == barr->size());

      return barr;
  }

  // Converts an Int array into its char* equivalent.
  const char* serialize(IntArray* iarr) {

      ByteArray* barr = new ByteArray();

      // serialize size
      barr->push_string("siz: ");
      const char* ser_sz = serialize(iarr->size());
      barr->push_string(ser_sz);
      delete[] ser_sz;

      // serialize array
      barr->push_string("\narr: ");
      for (size_t i = 0; i < iarr->size() - 1; ++i) {
          const char* ser_int = serialize((int)iarr->get(i));
          barr->push_string(ser_int);
          barr->push_string(", ");
          delete[] ser_int;
      }

      // add last element of array
      const char* ser_int = serialize((int)iarr->get(iarr->size() - 1));
      barr->push_string(ser_int);
      delete[] ser_int;

      const char* str = barr->as_bytes();
      delete barr;
      return str;
  }

  // Deserializes an IntArray.
  IntArray* get_int_array(const char* str) {
      size_t arr_size, new_line_loc, i;

      IntArray* iarr = new IntArray();

      // go through lines of str
      i = 0;
      while (i < strlen(str)) {
          //printf("%zu, %zu\n", i, strlen(str));
          // get the type of this line
          char type_buff[4];
          memcpy(type_buff, &str[i], 3);
          type_buff[3] = 0;
          // this is a sz line
          if (strcmp(type_buff, "siz") == 0) {
              i += 5;
              // find the end of the line
              for (size_t j = i; str[j] != '\n' && str[j] != 0; ++j) {
                  new_line_loc = j;
              }
              ++new_line_loc;
              // get the size
              char sz_buff[new_line_loc - i + 1];
              memcpy(sz_buff, &str[i], new_line_loc - i);
              sz_buff[new_line_loc - i] = 0;
              arr_size = atoi(sz_buff);
              i = new_line_loc + 1;
          // this is an ar line
          } else if (strcmp(type_buff, "arr") == 0) {
              // get the int array line
              char* ar_line = duplicate(&str[i + 5]);
              i += 5;
              // split into tokens and add to darr
              char* next_int;
              next_int = strtok(ar_line, ", ");
              while (next_int != NULL) {
                  i += strlen(next_int) + 2;
                  iarr->push_back(atof(next_int));
                  next_int = strtok(NULL, ", ");
              }
              delete[] ar_line;
          }
      }

      // make sure sizes match up
      assert(arr_size == iarr->size());

      return iarr;
  }

  // Converts a Key (from key value store) into its char* equivalent.
  const char* serialize(Key* key) {
    ByteArray* barr = new ByteArray();

    // serialize the home node
    barr->push_string("\t\t\t\tnde: ");
    const char* ser_nde = serialize(key->getHomeNode());
    barr->push_string(ser_nde);
    delete[] ser_nde;

    // serialize the key name
    barr->push_string("\n\t\t\t\tnme: ");
    const char* ser_nme = serialize(key->getName());
    barr->push_string(ser_nme);
    delete[] ser_nme;

    const char* str = barr->as_bytes();
    delete barr;
    return str;
  }

  // Serializes an array of Keys.
  const char* serialize(KeyArray* keys) {
      ByteArray* barr = new ByteArray();

      // serialize number of keys
      barr->push_string("\t\t\tsiz: ");
      const char* ser_siz = serialize(keys->size());
      barr->push_string(ser_siz);
      delete[] ser_siz;
      barr->push_back('\n');

      // serialize each key
      for (size_t i = 0; i < keys->size(); ++i) {
          barr->push_string("\t\t\tkey:\n");
          const char* ser_key = serialize(keys->get(i));
          barr->push_string(ser_key);
          if (i != keys->size() - 1) barr->push_back('\n');
          delete[] ser_key;
      }

      const char* str = barr->as_bytes();
      delete barr;
      return str;
  }

  // Deserializes a key.
  Key* get_key(const char* str, size_t* ii) {
    // make sure node line
    size_t i = 0;
    char type_buff[4];
    memcpy(type_buff, &str[i], 3);
    type_buff[3] = 0;
    if (strcmp(type_buff, "nde") != 0) return nullptr;

    size_t nde = get_size(&str[i], &i);

    // name line
    i += 4;
    memcpy(type_buff, &str[i], 3);
    type_buff[3] = 0;
    if (strcmp(type_buff, "nme") != 0) return nullptr;

    // find the end of the line
    i += 6;
    size_t new_line_loc = 0;
    for (size_t j = i; str[j] != '\n' && str[j] != 0; ++j) {
      new_line_loc = j;
    }
    ++new_line_loc;

    // get the name
    char buff[new_line_loc - i];
    memcpy(buff, &str[i], new_line_loc - i - 1);
    buff[new_line_loc - i - 1] = 0;

    Key* k = new Key(new String(buff), (int)nde);

    (*ii) += new_line_loc + 1;

    return k;
  }

  // Deserializes a KeyArray.
  KeyArray* get_key_array(const char* str, size_t* ii) {
    size_t i = 0;
    size_t size;

    // make sure type of this line is kys
    char type_buff[4];
    memcpy(type_buff, &str[i], 3);
    type_buff[3] = 0;
    if (strcmp(type_buff, "kys") != 0) return nullptr;

    // get number of keys
    i += 8;
    memcpy(type_buff, &str[i], 3);
    type_buff[3] = 0;
    if (strcmp(type_buff, "siz") != 0) return nullptr;
    size = get_size(&str[i], &i);

    KeyArray* karr = new KeyArray();

    // get keys
    for (size_t j = 0; j < size; ++j) {
      i += 12;
      Key* k = get_key(&str[i], &i);
      karr->push_back(k);
    }

    (*ii) += i;

    return karr;
  }
};
