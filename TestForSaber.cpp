#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
using std::cout;
using std::endl;




struct ListNode
{
    ListNode* Prev;
    ListNode* Next;
    ListNode* Rand; // произвольный элемент внутри списка
    std::string Data;
};


struct ListRand
{
    ListNode* Head;
    ListNode* Tail;
    uint32_t Count = 0;

    void Serialize(std::ofstream& s);
    void Deserialize(std::ifstream& s);

    void PushBack(std::string data) {
        if (!Count) {
            Head = Tail = new ListNode();
            Head->Data = data;
        }
        else {
            Tail->Next = new ListNode();
            Tail->Next->Prev = Tail;
            Tail = Tail->Next;
            Tail->Next = nullptr;
            Tail->Data = data;
        }
        ++Count;
    }
    void Print() {
        ListNode* current = Head;
        while (current) {
            std::cout << current->Data << " ";
            current = current->Next;
        }
        cout << endl;
    }
    void PrintByRand() {
        ListNode* current = Head;
        while (current) {
            std::cout << current->Data << " ";
            current = current->Rand;
        }
        cout << endl;
    }
};

struct NodeAsPrimitive {
    uint32_t ThisIndex;
    uint32_t RandIndex;
    uint32_t DataSize;
    std::string Data;
    static NodeAsPrimitive* create(std::string Data, uint32_t ThisIndex, uint32_t RandIndex) {
        NodeAsPrimitive* current = new NodeAsPrimitive();
        current->DataSize = Data.size(); // char = 8 bit => string = str.size() byte
        current->Data = Data;
        current->ThisIndex = ThisIndex;
        current->RandIndex = RandIndex;
        return current;
    }
};

class ListEncoder { // Класс кодировщик для для списка
private:
    // Ввёл MyPair, чтобы можно было отсортировать массив таких пар по первому значению
    template<class _First, class _Second>
    struct MyPair : public std::pair<_First, _Second> {
        MyPair(_First First, _Second Second)
            : 
            std::pair<_First, _Second>(First, Second) {}
        bool operator<(const std::pair<_First, _Second>& other) {
            return this->first < other.first;
       }
    };
    friend uint32_t BinaryFind(std::vector<ListEncoder::MyPair<uint64_t, uint32_t>>::iterator _First, 
                               std::vector<ListEncoder::MyPair<uint64_t, uint32_t>>::iterator _Last, 
                               const uint64_t& _FirstVal);
    static std::vector<MyPair<uint64_t, uint32_t>>  ThisForThis;        // ассоциативный массив где current index - key, current address && current index - value
    static std::vector<NodeAsPrimitive*>            PrimitiveNodes;     // "список" составленный из примитивов
    static std::vector<uint8_t>                     ByteCode;           // вектор-набор байтов (закодированная инфа)
    template<typename T>
    static void encodeInt(std::vector<uint8_t>& buff, T data) {
        for (size_t i = 1; i <= sizeof(data); i++)
            buff.push_back(data >> (sizeof(data) - i) * 8);
    }
    static void encodeString(std::vector<uint8_t>& buff, std::string data) {
        for (auto& character : data)
            buff.push_back(character);
    }
public:
    static std::vector<uint8_t>& getByteCode() { return ByteCode; }
    static void FillAssociatedArray(ListRand& list) {       // 1) Заполнение ассоциативного массива ThisForThis
        ListNode* current = list.Head;
        uint32_t index = 0;
        ThisForThis.push_back(MyPair<uint64_t, uint32_t>(NULL,NULL));
        while (current) {                                   // 1.1) вставляем адрес ячейки (этакий уникальный id) и её индекс 
            // (хотя это скорее явлеятся позицией, потмоу как в 0 элементе будет хранится 0, для восстановления связи с nullptr)
            ThisForThis.push_back(MyPair<uint64_t, uint32_t>(uint64_t(current), index + 1));
            current = current->Next; ++index;
        }                                                   // Сложность блока - O(n)
        std::sort(ThisForThis.begin(), ThisForThis.end());  // 1.2) сортируем для последующего бинарного поиска
                                                            // сложность std::sort = O(n*log(n))
    }
    static void NodesPrimitivation(ListRand& list) {                    // 2) Создание вектора "примитивов"
        if (ThisForThis.empty()) return;
        ListNode* current = list.Head;
        PrimitiveNodes.push_back(nullptr);
        for (uint32_t index = 0; index < list.Count; ++index) {         // 2.1) Ищем индекс рандомного элемента по ассоциации
                                                                        //      с индексом текущего. Сложность поиска O(log(n))
            uint32_t RandIndex = BinaryFind(ThisForThis.begin(), ThisForThis.end(), uint64_t(current->Rand));
            PrimitiveNodes.push_back(NodeAsPrimitive::create(current->Data, index, RandIndex));
            current = current->Next;                                    // 2.2) Заполняем массив примитивов
        }                                                               //      Сложность блока = O(n*log(n))
    }
    static void TranslateToByteCode() {                                 // 3) Создание байт кода
        for (auto& node : PrimitiveNodes) {
            if (node) {
                encodeInt<uint32_t>(ByteCode, node->ThisIndex);             // 3.1) Запись текущего индекса в байт код
                encodeInt<uint32_t>(ByteCode, node->RandIndex);             // 3.2) Запись рандомного индекса в байт код
                encodeInt<uint32_t>(ByteCode, node->DataSize);              // 3.3) Запись размера строки в байт код
                encodeString(ByteCode, node->Data);                         // 3.4) Запись строки в байт код
            }
        }                                                               // Сложность блока O(n)
    }
    static void ClearEncoder() {
        ThisForThis.clear();
        PrimitiveNodes.clear();
        ByteCode.clear();
    }
};

// инициализация статических полей
std::vector<ListEncoder::MyPair<uint64_t, uint32_t>> ListEncoder::ThisForThis;
std::vector<NodeAsPrimitive*> ListEncoder::PrimitiveNodes;
std::vector<uint8_t> ListEncoder::ByteCode;

uint32_t BinaryFind(std::vector<ListEncoder::MyPair<uint64_t, uint32_t>>::iterator _First, 
                    std::vector<ListEncoder::MyPair<uint64_t, uint32_t>>::iterator _Last, 
                    const uint64_t& _FirstVal) 
{
    auto _Mid = _First + std::distance(_First, _Last) / 2;
    if (_FirstVal == (*_Mid).first) return (*_Mid).second;
    if (_First == _Last) return 0;
    if (_FirstVal < (*_Mid).first) return BinaryFind(_First, _Mid, _FirstVal);
    else return BinaryFind(_Mid, _Last, _FirstVal);

}

void ListRand::Serialize(std::ofstream& s) {        // Сериализация. Выполняется в 4 этапа:
    ListEncoder::FillAssociatedArray(*this);        // 1) Заполнение ассоциативного массива ThisForThis
    ListEncoder::NodesPrimitivation(*this);         // 2) Созданиe вектора "примитивов"
    ListEncoder::TranslateToByteCode();             // 3) Создание байт кода
    for (auto& byte : ListEncoder::getByteCode())   // 4) Запись байт кода в файл
        s << byte;                                  // Сложность записи O(n)
    ListEncoder::ClearEncoder();                    // Сброс енкодера

    // Сложность всего алгоритма сериализации не превышает O(n*log(n))
}

class ListDecoder {
private:
    static std::vector<NodeAsPrimitive*>            PrimitiveNodes;     // "список" составленный из примитивов
    static std::vector<uint32_t>                    ThisForRand;
    static std::vector<ListNode*>                   Nodes;
    static std::vector<uint8_t>                     ByteCode;           // вектор-набор байтов (закодированная инфа)
    template<typename T>
    static T decodeInt(std::vector<uint8_t> &buff) {
        T number = 0;
        for (auto& character : buff) {
            number <<= 8;
            number |= character;
        }
        return number;
    }
    static std::string decodeString(std::vector<uint8_t> &buff) {
        std::string data;
        for (auto& character : buff)
            data.push_back(character);
        return data;
    }
public:
    static void TranslateToPrimitive(std::vector<uint8_t> &ByteCode) {                  // 2) Создание вектора примитивов
        uint32_t index = 0, DataSize = 0;
        std::vector<uint8_t> buff;
        PrimitiveNodes.push_back(nullptr);
        for (auto& byte : ByteCode) {
            ++index;
            buff.push_back(byte);
            if (index == 4) {                                                           // 2.1) Чтение текущего индекса
                PrimitiveNodes.push_back(new NodeAsPrimitive());
                PrimitiveNodes.back()->ThisIndex = decodeInt<uint32_t>(buff);
                buff.clear();
            }
            if (index == 8) {                                                           // 2.2) Чтение рандомного индекса
                PrimitiveNodes.back()->RandIndex = decodeInt<uint32_t>(buff);
                buff.clear();
            }
            if (index == 12) {                                                          // 2.3) Чтение размера строки
                PrimitiveNodes.back()->DataSize = DataSize = decodeInt<uint32_t>(buff);
                buff.clear();
            }
            if (index == 12 + DataSize) {                                               // 2.4) Чтение строки
                PrimitiveNodes.back()->Data = decodeString(buff);
                buff.clear(); index = 0;
            }
        }
                                                                                        // Сложность блока O(n)
    }
    static ListRand* BuildList() {                          // 3) Конструирование связного спика
        ListRand* NewList = new ListRand();
        ThisForRand.push_back(NULL);
        Nodes.push_back(nullptr);
        for (auto& node : PrimitiveNodes) {
            if (node) {
                NewList->PushBack(node->Data);                  // 3.1) Добавление элементов в список
                ThisForRand.push_back(node->RandIndex);         // 3.2) Заполнение ассоциативного массива индекс - индекс
                Nodes.push_back(NewList->Tail);                 // 3.3) Заполнение ассоциативного массива индекс - узел
            }
        }                                                   // Сложость блока O(n)
        return NewList;
    }
    static void setRand(ListRand& list) {                           // 4) Восстановление рандомных ссылок
        ListNode* current = list.Head;
        for (uint32_t index = 1; index <= list.Count; ++index) {
            current->Rand = Nodes[ThisForRand[index]];              // 4.1) Собственно восстановление рандомных ссылок
            current = current->Next;
        }                                                           // Сложность блока O(n)
    }
    static void ClearDecoder() {
        PrimitiveNodes.clear();
        ThisForRand.clear();
        Nodes.clear();
        ByteCode.clear();
    }
};

// инициализация статических полей
std::vector<NodeAsPrimitive*> ListDecoder::PrimitiveNodes;
std::vector<uint32_t> ListDecoder::ThisForRand;
std::vector<ListNode*> ListDecoder::Nodes;
std::vector<uint8_t> ListDecoder::ByteCode;

void ListRand::Deserialize(std::ifstream& s) {      // Десериализация проходит за 4 этапа:
    std::vector<uint8_t> ByteCode;
    char character;
    while (s.get(character))                        // 1) Получение байт кода из файла
        ByteCode.push_back(character);              // Сложность блока O(n)
    ListDecoder::TranslateToPrimitive(ByteCode);    // 2) Создание вектора примитивов
    *this = *ListDecoder::BuildList();           // 3) Конструирование связного спика
    ListDecoder::setRand(*this);                    // 4) Восстановление рандомных ссылок
    ListDecoder::ClearDecoder();                    // Сброс декодера
    // Сложность всего алгоритма десериализации не превышает O(n)
}

int main() {
    setlocale(LC_ALL, "");

    ListRand a;
    {
        a.PushBack("проверка");
        a.PushBack("алгоритма");
        a.PushBack("сериализации");
        a.PushBack("и");
        a.PushBack("десереализации");
        a.Head->Rand = a.Tail;
        a.Tail->Rand = a.Tail->Prev;
        a.Tail->Prev->Rand = a.Head->Next->Next;
        a.Head->Next->Next->Rand = a.Head->Next;
    }
    cout << "Линейный вывод: "; a.Print();
    cout << "Рандомный вывод: "; a.PrintByRand();
    std::ofstream out("save.bin");
    a.Serialize(out);
    out.close();

    cout << endl;

    ListRand b;
    std::ifstream in("save.bin");
    b.Deserialize(in);
    in.close();
    cout << "Линейный вывод: "; b.Print();
    cout << "Рандомный вывод: "; b.PrintByRand();

    std::cout << std::endl;
    system("pause");
}


