#pragma once

#include "dataframe.h"
#include "application.h"
#include "string.h"
#include "array.h"

/**
  * Runs the trivial milestone.
  * Shows that a dataframe can be populated and stored in a KV store.
  * Then that dataframe can be retrieved out of the KV store.
  * @authors course staff, horn.s@husky.neu.edu, armani.a@husky.neu.edu
  */
class Trivial : public Application {
public:

  Key* key_int;
  Key* key_double;
  DataFrame* d;

  Trivial(size_t idx, KVStore* kv) : Application(idx, kv) {
    String* s1 = new String("int-triv");
    String* s2 = new String("double-triv");
    key_int = new Key(s1, 0);
    key_double = new Key(s2, 0);
    Schema s("");
    d = new DataFrame(s, kv_);
    run_();
  }

  ~Trivial() {
    delete key_int;
    delete key_double;
    delete d;
  }

  // Runs both the int and double array versions of the example.
  void run_() {
    run_int();
    printf("Trivial example for IntArray complete and successful.\n");
    run_double();
    printf("Trivial example for DoubleArray complete and successful.\n\n");
  }

  void run_int() {
    size_t SZ = 1000*1000;
    IntArray* vals = new IntArray();

    int sum = 0;
    for (size_t i = 0; i < SZ; ++i) {
      vals->push_back(i);
      sum += vals->get(i);
    }

    DataFrame* df = d->from_array(key_int, getKVStore(), SZ, vals);

    assert(df->get_int(0,1) == 1);

    DataFrame* df2 = getKVStore()->get(key_int);
    for (size_t i = 0; i < SZ; ++i) sum -= df2->get_int(0, i);

    assert(sum == 0);

    delete vals;
    delete df;
    delete df2;
  }

  void run_double() {
    size_t SZ = 1000*1000;
    DoubleArray* vals = new DoubleArray();

    double sum = 0;
    for (size_t i = 0; i < SZ; ++i) {
      vals->push_back(i);
      sum += vals->get(i);
    }

    DataFrame* df = d->from_array(key_double, getKVStore(), SZ, vals);

    assert(df->get_double(0,1) == 1);

    DataFrame* df2 = getKVStore()->get(key_double);
    for (size_t i = 0; i < SZ; ++i) sum -= df2->get_double(0, i);

    assert(sum == 0);

    delete vals;
    delete df;
    delete df2;
  }
};
