
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

struct Base
{
    Base() {}
    virtual ~Base() {}
};

struct Derived : public Base
{
    Derived() : Base() {}
    virtual ~Derived() {}
};

struct Other : public Base
{
    Other() : Base() {}
    virtual ~Other() {}
};

void func(std::shared_ptr<Base> type)
{
    if (auto templ = std::dynamic_pointer_cast<Derived>(type))
    {
        std::cout << "Derived" << std::endl;
    }
    else if (auto templ = std::dynamic_pointer_cast<Other>(type))
    {
        std::cout << "Other" << std::endl;
    }
    else
    {
        std::cout << "Base" << std::endl;
    }
}

int main()
{
    func(std::make_shared<Derived>());
    func(std::make_shared<Other>());
    func(std::make_shared<Base>());
}