#pragma once

#include "dataframe.h"
#include "application.h"
#include "string.h"
#include "array.h"
#include "key.h"
#include "thread.h"

/**
  * Threaded network implementation of Milestone 3.
  * Demo.cpp contains the files necessary to run the non-threaded version.
  * Creates several keys, then checks to make sure they are present in a KeyValue Store.
  * The KeyValue store in this instance maps keys to dataframes.
  * @authors horn.s@husky.neu.edu, armani.a@husky.neu.edu, Course Staff (example code).
  */
class Demo : public Application {
public:

  DataFrame* df;
  DataFrame* df2;
  DataFrame* df3;

  String* m = new String("main");
  String* v = new String("verif");
  String* c = new String("ck");
  Key* main = new Key(m,0);
  Key* verify = new Key(v,0);
  Key* check = new Key(c,0);

  Demo(size_t idx, KVStore* kv): Application(idx, kv) {
    run_();
  }

  ~Demo() {
    delete m;
    delete v;
    delete c;
    delete main;
    delete verify;
    delete check;
  }

  void run_() override {
    switch(this_node()) {
    case 0:   producer();     break;
    case 1:   counter();      break;
    case 2:   summarizer();
   }
  }

  // Generates a Dataframe with 100*1000 elements. Sums it. Stores both in KV.
  void producer() {
    DoubleArray* vals = new DoubleArray();
    size_t SZ = 100*1000;

    double sum = 0;
    for (size_t i = 0; i < SZ; ++i) {
      vals->push_back(i);
      sum += vals->get(i);
    }

    df = df->from_array(main, getKVStore(), SZ, vals);
    df2 = df2->from_scalar(check, getKVStore(), sum);
    delete vals;
  }

  // Verifies the sum and creates another dataframe holding it.
  void counter() {
    size_t SZ = 100*1000;
    DataFrame* v = getKVStore()->getAndWait(main);

    double sum = 0;
    for (size_t i = 0; i < SZ; ++i) {
      sum += v->get_double(0,i);
    }
    p("The sum is  ").pln(sum);
    delete v;

    df3 = df3->from_scalar(verify, getKVStore(), kc_, sum);
  }

  // Checks that the original dataframe sum matches the new sum dataframe.
  void summarizer() {
    DataFrame* result = getKVStore()->getAndWait(verify);
    DataFrame* expected = getKVStore()->getAndWait(check);
    pln(expected->get_double(0,0)==result->get_double(0,0) ? "SUCCESS\n":"FAILURE\n");
    delete result;
    delete expected;
  }
};

/** A DemoThread wraps a Thread and contains a Demo.
 *  Necessary for the distributed portion of the KeyValue store.
 *  One thread can insert a key into a dataframe, the other thread can check
 *  the key is there.
 *  @authors: armani.a@husky.neu.edu, horn.s@husky.neu.edu */
class DemoThread : public Thread {
public:

  std::thread thread_;
  Demo* d;
  size_t threadId_;

  DemoThread(size_t index, size_t cur_thread, KVStore* kv) {
    threadId_ = cur_thread;
    d = new Demo(index, kv);
  }

  ~DemoThread() {
    delete d;
  }

};
