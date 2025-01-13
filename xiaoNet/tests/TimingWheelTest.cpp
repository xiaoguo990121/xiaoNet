#include <xiaoNet/utils/TimingWheel.h>
#include <xiaoLog/Logger.h>
#include <memory>

class MyClass
{
public:
    MyClass();
    MyClass(MyClass &&) = default;
    MyClass(const MyClass &) = default;
    MyClass &operator=(MyClass &&) = default;
    MyClass &operator=(const MyClass &) = default;
    ~MyClass();

private:
};

MyClass::MyClass()
{
}

MyClass::~MyClass()
{
    LOG_DEBUG << "MyClass destructed!";
}

int main()
{
    LOG_DEBUG << "start";
    xiaoNet::EventLoop loop;
    std::weak_ptr<MyClass> weakEntry;
    xiaoNet::TimingWheel wheel(&loop, 75, 0.1, 100);
    {
        auto entry = std::shared_ptr<MyClass>(new MyClass);

        wheel.insertEntry(75, entry);
        weakEntry = entry;
    }

    loop.loop();
}