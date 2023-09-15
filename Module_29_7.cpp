#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <cassert>


struct Node
{
    int value;
    Node* next;
    std::mutex* node_mutex;
    Node(int value, Node* next) : value(value), next(next) {
        node_mutex = new std::mutex();
    }
};

class FineGrainedQueue
{
private:
    Node* head;
    std::mutex* queue_mutex;
public:
    FineGrainedQueue();
    int getValue(int pos)const;                 // Функция возвращающая значение узла списка по позиции (индексу)
    int getIndex(int value)const;               // Функция возвращающая позицию (индекс) узла списка по значению
    void insertIntoMiddle(int value, int pos);
    void remove(int value);                     // Функция удаления узла списка
    int size()const;                            // Функция возвращающая размер списка
};

FineGrainedQueue::FineGrainedQueue() : head(nullptr), queue_mutex(new std::mutex) {

}

int FineGrainedQueue::getValue(int pos)const {
    queue_mutex->lock();
    auto count = 0;
    auto node = head;
    while (node != nullptr) {
        if (count == pos)
        {
            node->node_mutex->lock();
            queue_mutex->unlock();
            auto val = node->value;
            node->node_mutex->unlock();
            return val;
        }
        ++count;
        node = node->next;
    }
    queue_mutex->unlock();
    return { };
}

int FineGrainedQueue::getIndex(int value)const {
    std::lock_guard<std::mutex> lock(*queue_mutex);
    auto count = 0;
    auto node = head;
    while (node != nullptr) {
        std::lock_guard<std::mutex> lock(*node->node_mutex);
        if (node->value == value)
            return count;
        ++count;
        node = node->next;
    }
    return -1;
}

int FineGrainedQueue::size()const {
    auto count = 0;
    std::lock_guard<std::mutex> lock(*queue_mutex);
    auto node = head;
    while (node != nullptr) {
        ++count;
        node = node->next;
    }
    return count;
}

// Реализация функции вставки в список значения по позиции 
void FineGrainedQueue::insertIntoMiddle(int value, int pos) {
    std::lock_guard<std::mutex> lock(*queue_mutex);

    auto cur = this->head;

    if (pos == 0) {
        auto node = new Node(value, cur);
        this->head = node;
    }
    else {
        size_t count = 0;
        auto prev = this->head;
        cur = this->head->next;
        while (cur != nullptr) {
            count++;
            if (count == pos) {
                auto node = new Node(value, cur);
                prev->next = node;
            }
            prev = cur;
            cur = cur->next;
        }
    }

}

void FineGrainedQueue::remove(int value)
{
    Node* prev, * cur;
    queue_mutex->lock();

    // здесь должна быть обработка случая пустого списка

    prev = this->head;
    cur = this->head->next;

    prev->node_mutex->lock();
    // здесь должна быть обработка случая удаления первого элемента
    queue_mutex->unlock();

    if (cur) // проверили и только потом залочили
        cur->node_mutex->lock();
    while (cur)
    {
        if (cur->value == value)
        {
            prev->next = cur->next;
            prev->node_mutex->unlock();
            cur->node_mutex->unlock();
            delete cur;
            return;
        }
        Node* old_prev = prev;
        prev = cur;
        cur = cur->next;
        old_prev->node_mutex->unlock();
        if (cur) // проверили и только потом залочили
            cur->node_mutex->lock();
    }
    prev->node_mutex->unlock();
}

// Модифицирую список для проверки корректности работы программы
void thread1(FineGrainedQueue& queue) {
    queue.insertIntoMiddle(51, 0);
    queue.remove(18);
    queue.insertIntoMiddle(52, 0);

    auto const size = queue.size();

    for (auto i = 0; i < 100; ++i) {
        queue.getIndex(rand() % size);
        queue.getValue(rand() % size);
    }
}

// Модифицирую список для проверки корректности работы программы
void thread2(FineGrainedQueue& queue) {
    queue.remove(17);
    queue.insertIntoMiddle(93, 1);
    queue.insertIntoMiddle(94, 3);

    auto const size = queue.size();

    for (auto i = 0; i < 100; ++i) {
        queue.getIndex(rand() % size);
        queue.getValue(rand() % size);
    }
}

int main()
{
    FineGrainedQueue queue;         //  Создаю список
    queue.insertIntoMiddle(15, 0);  //  Вставляю значение 15 в корневой узел
    queue.insertIntoMiddle(16, 0);  //  Вставляю значение 16 в корневой узел, значение 15 при этом должно сдвинуться вправо на 1 позицию
    queue.insertIntoMiddle(17, 0);  //  и т.д.
    queue.insertIntoMiddle(18, 0);  //  и т.д.

    assert(queue.size() == 4);        //  Размер списка должен быть равен 4. Проверяю так ли это.
    assert(queue.getValue(0) == 18);  //  Проверяю значение в корневом узле. Оно должно быть равно 18.
    assert(queue.getValue(1) == 17);  //  Проверяю значения узлов на других позициях.
    assert(queue.getValue(2) == 16);  //
    assert(queue.getValue(3) == 15);  //

    std::thread thr1(thread1, std::ref(queue));  //  1 поток
    std::thread thr2(thread2, std::ref(queue));  //  2 поток
    thr1.join();
    thr2.join();

    // Проверяю правильность работы программы в многопоточном режиме
    assert(queue.getValue(0) == 52);   //  Значение в корневом узле должно быть 52
    assert(queue.size() == 6);         //  Размер списка должен быть равен 6
    assert(queue.getIndex(93) != -1);  //  Узел со значением 93 должен присутствовать в списке
    assert(queue.getIndex(94) != -1);  //  Узел со значением 94 должен присутствовать в списке
}
