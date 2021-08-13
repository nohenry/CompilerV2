
#include <memory>
#include <iostream>

// void *operator new(size_t size)
// {
//     std::cout << "Allocated " << size << " bytes" << std::endl;
//     return std::malloc(size);
// }

// void operator delete(void *ptr) noexcept
// {
//     std::cout << "Destroyed " << ptr << std::endl;
//     return std::free(ptr);
// }

struct Object
{
    int i;
    Object(int i) : i(i)
    {
        std::cout << "Created Object " << this << std::endl;
    }
    ~Object()
    {
        std::cout << "Destroyed Object " << this << std::endl;
    }
};

struct Other
{
    std::shared_ptr<Object> obj;
    Other(std::shared_ptr<Object> obj) : obj(obj) {}

    void SetObject(std::shared_ptr<Object> obj)
    {
        this->obj = obj;
    }
};

std::unique_ptr<Object> func()
{
    return std::make_unique<Object>(6);
}

int main()
{
    auto s = func();
    // auto o = s;

    std::cout << s << std::endl;
}