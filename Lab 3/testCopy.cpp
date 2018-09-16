#include "alang.hpp"
#include <iostream>
#include <cassert>

using namespace std;

template <typename E> class queue;    // forward declaration 
template <typename E> class node_iterator;  // forward declaration

template <typename E>
class node {
  E data;
  A<node<E>*> next;
  node(E d, node<E>* n) : data(d), next(n) {}
public:

  friend class queue<E>;         // allow queue<E> to access private members
  friend class node_iterator<E>; // allow node_iterator<E> to access private members
};

template <typename E>
class node_iterator {
  node<E>* it;
public:
  node_iterator(node<E>* n) : it(n) {}
  node_iterator& operator++() { assert(it != nullptr); it = it->next; return *this; }
  bool done() const { return it == nullptr; }
  E operator*() const { assert(it != nullptr); return it->data; }
};

template <typename E> 
class queue {
  A<node<E>*> head;
  A<node<E>*> rear;
  A<int> _size;
public:
  queue() : head(nullptr), rear(nullptr), _size(0) {}

  bool empty() const { return _size.read() == 0; }
  // alternatively we could test for head or rear being equal to nullptr

  void enqueue(E d) {
    node<E>* newNode = new node<E>(d, nullptr);
    ATO
      if (rear == nullptr) { // enqueueing the first element
        head = newNode;
      } else {
        rear.read()->next = newNode;
      }
      rear = newNode;
      _size = _size + 1;
    MIC;

    // ATO _size = _size + 1; MIC; <--
  }

  E dequeue() {
    node<E>* oldHead;

    // Modify shared variables
    ATO
      if (empty()) throw "empty queue";
      oldHead = head;
      head = head.read()->next;
      if (head.read() == nullptr) rear = nullptr; // removed the last element
      _size = _size - 1;
    MIC;

    // Extract data and delete the node
    E e = oldHead->data;
    delete oldHead;
    return e;
  }

  int size() const {
    return _size.read(); 
  }

  node_iterator<E> iterator() {
    return head.read();
  }
};

int main() {

  queue<int> q;
  int nThreads = 1000;
  int nIncs = 100;
  A<int> expected = 0;

  {
    processes ps;

    // Lots of enqueueing with some dequeueing
    for (int i : range(0, nThreads)) {
      ps += [&, i] {
        for (int j : range(0, nIncs)) {
          q.enqueue(i);
          ATO expected = expected + 1; MIC;
          if (j%2 == 0) {
            q.dequeue();
            ATO expected = expected - 1; MIC;
          }
        }
      };
    }
  }

  int sz = q.size(); // Check the reported size of the queue
  int actual = 0; // Count the actual amount of elements
  for (auto it = q.iterator(); !it.done(); ++it) actual++;

  // Dequeue the elements
  int successes = 0;
  while (true) {
    try {
      q.dequeue();
      successes++;
    } catch (...) {
      break;
    }
  }

  std::cout
    << "Expected: " << expected << std::endl
    << ".size():  " << sz << std::endl
    << "Iterated: " << actual << std::endl
    << "Dequeued: " << successes << std::endl;
}
