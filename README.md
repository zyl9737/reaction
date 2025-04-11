# ReactiveCpp: Modern C++ Reactive Programming Framework

[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Header-only](https://img.shields.io/badge/Header--only-Yes-green.svg)](https://en.wikipedia.org/wiki/Header-only)
[![CMake](https://img.shields.io/badge/CMake-3.15+-blueviolet.svg)](https://cmake.org)

A lightweight, header-only reactive programming framework leveraging modern C++20 features for building efficient dataflow applications.

### 🚀 Performance Optimized

- **Zero-cost abstractions** through template metaprogramming
- **Compile-time dependency graph** optimization
- Minimal runtime overhead with **smart change propagation**

### 🔗 Intelligent Dependency Management

- Automatic **DAG detection** and cycle prevention
- Fine-grained **change propagation control**
- Configurable **caching strategies**

### 🛡️ Type Safety Guarantees

- Compile-time **type checking** with C++20 concepts
- Safe **value semantics** throughout the framework

### 🧩 Extensible Design

| Feature          | Options                          |
|------------------|----------------------------------|
| Trigger Policy   | ValueChange, Threshold, Timer, Custom |
| Invalidation     | Direct, KeepCalculate, LastValue |

### 📦 Requirements

- **Compiler**: C++20 compatible (GCC 10+, Clang 12+, MSVC 19.30+)
- **Build System**: CMake 3.15+

## 🛠 Installation

To build and install the `reaction` reactive framework, follow the steps below:

```bash
git clone https://github.com/lumia431/reaction.git && cd reaction
cmake -B build && cd build
cmake --build .
cmake --install . --prefix /your/install/path
```

After installation, you can include and link against reaction in your own CMake-based project:

```cmake
find_package(reaction REQUIRED)
target_link_libraries(your_target PRIVATE reaction)
```

### 🚀 Quick Start

```cpp
#include <reaction/dataSource.h>
#include <iostream>
#include <iomanip>

int main() {
    using namespace reaction;

    // 1. Create reactive variables
    auto buyPrice = var(100.0);      // Purchase price
    auto currentPrice = var(105.0);  // Market price

    // 2. Derived calculations
    auto profit = calc([=] {
        return currentPrice() - buyPrice();
    });

    // 3. Expressive formulas
    auto profitPercent = expr((currentPrice - buyPrice) / buyPrice * 100);

    // 4. Automatic reactions
    auto logger = action([=] {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "[Stock Update] Current: $" << currentPrice()
                  << " | Profit: $" << profit()
                  << " (" << profitPercent() << "%)" << std::endl;
    });

    // Simulate market changes
    currentPrice.value(110.0);  // Price increase
    currentPrice.value(95.0);   // Price drop
    *buyPrice = 90.0;           // Adjust basis
    return 0;
}
```

### 📖 Basic Usage

#### 1. Reactive Variables: `var`

Define reactive state variables with `var<T>`.

```cpp
auto a = reaction::var(1);         // int variable
auto b = reaction::var(3.14);      // double variable
```

- Method-style get value:

```cpp
auto val = a.get();
```

- brief way:

```cpp
auto val = a();
```

- Method-style assignment:

```cpp
a.value(2);
```

- Pointer-style assignment:

```cpp
*a = 2;
```

#### 2. Derived Computation: calc

Use **calc** to create reactive computations based on one or more var instances.

- Lambda Capture Style:

```cpp
auto a = reaction::var(1);
auto b = reaction::var(3.14);
auto sum = reaction::calc([=]() {
    return a() + b();  // Retrieve current values using a() and b()
});
```

- Parameter Binding Style (High Performance):

```cpp
auto ds = reaction::calc([](auto aa, auto bb) {
    return std::to_string(aa) + std::to_string(bb);
}, a, b);  // Dependencies: a and b
```

#### 3. Declarative Expression: expr

expr provides a clean and concise syntax to declare reactive expressions. The result automatically updates when any dependent variable changes.

```cpp
auto a = reaction::var(1);
auto b = reaction::var(2);
auto result = reaction::expr(a + b * 3);  // result updates automatically when 'a' or 'b' change
```

#### 4. Reactive Side Effects: action

Register actions to perform side effects whenever the observed variables change.

```cpp
int val = 10;
auto a = reaction::var(1);
auto dds = reaction::action([&val]() {
    val = a();
});
```

Ofcourse, to get high performance can use Parameter Binding Style.

```cpp
int val = 10;
auto a = reaction::var(1);
auto dds = reaction::action([&val](auto aa) {
    val = aa;
}, a);
```

#### 5. Reactive Struct Fields: `Field`

For complex types with reactive fields allow you to define struct-like variables whose members are individually reactive.

Here's an example of a `PersonField` class:

```cpp
class PersonField : public reaction::FieldStructBase {
public:
    PersonField(std::string name, int age):
        m_name(reaction::field(this, name)),
        m_age(reaction::field(this, age)){}

    std::string getName() const { return m_name.get(); }
    void setName(const std::string &name) { *m_name = name; }
    int getAge() const { return m_age.get(); }
    void setAge(int age) { *m_age = age; }

private:
    reaction::Field<std::string> m_name;
    reaction::Field<int> m_age;
};

auto p = reaction::var(PersonField{"Jack", 18});
auto action = reaction::action(
    []() {
        std::cout << "Action Trigger , name = " << p().getName() << " age = " << p().getAge() << '\n';
    });

p->setName("Jackson"); // Action Trigger
p->setAge(28);         // Action Trigger
```

#### 6. Resetting Nodes and Dependencies

The reaction framework allows you to **reset a computation node** by replacing its computation function.
This mechanism is useful when the result needs to be recalculated using a different logic or different dependencies after the node has been initially created.

``Note:`` **The return value type cannot be changed**

Below is an example that demonstrates the reset functionality:

```cpp
TEST(TestReset, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(std::string{"2"});
    auto ds = reaction::calc([]() { return std::to_string(a()); });
    ds.set([=]() { return b() + "set"; });
}
```

#### 7. Trigger Mode

The `reaction` framework supports various triggering mode to control when reactive computations are re-evaluated. This example demonstrates three mode:

1. **Value Change Trigger:** The reactive computation is triggered only when the underlying value actually changes.
2. **Threshold Trigger:** The reactive computation is triggered when the value crosses a specified threshold.
3. **Always Trigger:** (Not explicitly shown in this example) Always triggers regardless of whether the value has changed.

The trigger Mode can be specified by the type parameter

```cpp
using namespace reaction;
auto stockPrice = var(100.0);
auto profit = expr<ValueChangeTrigger>(stockPrice() - 100.0);
auto assignAction = action([=]() {  // defalut AlwaysTrigger
    std::cout << "Checky assign, price = " << stockPrice() <<'\n';
});
auto sellAction = action<ThresholdTrigger>([=]() {
    std::cout << "It's time to sell, profit = " << profit() <<'\n';
});
sellAction.setThreshold([=]() {
    return profit() > 5.0;
});
*stockPrice = 100.0; // assignAction trigger
*stockPrice = 101.0; // assignAction, profit trigger
*stockPrice = 106.0; // all trigger

```

You can even define a trigger mode yourself in your code, just include the **checkTrigger** method:

```cpp
struct MyTrigger {
    bool checkTrigger() {
        // do something
        return true;
    }
};
auto a = var(1);
auto b = expr<MyTrigger>(a + 1);
```

#### 8. Invalid Strategies

In the `reaction` framework, all data sources **obtained by users are actually in the form of weak references**, and their actual memory is managed **in the observer map**.
Users can manually call the **close** method, so that all dependent data sources will also be closed.

```cpp
auto a = reaction::var(1);
auto b = reaction::var(2);
auto dsA = reaction::calc([=]() { return a(); });
auto dsB = reaction::calc([=]() { return dsA() + b(); });
dsA.close(); //dsB will automatically close, cause dsB dependents dsA.
EXPECT_FALSE(static_cast<bool>(dsA));
EXPECT_FALSE(static_cast<bool>(dsB));
```

However, for scenarios where the lifecycle of a weak reference acquired by user ends, the `reaction` framework makes several strategy for different scenarios.

- **DirectCloseStrategy:**
  The node is immediately closed (made invalid) when any of its dependencies become invalid.

- **KeepCalcStrategy:**
  The node continues to recalculate, its dependencies work normally.

- **LastValStrategy:**
  The node retains the last valid, its dependencies use the value to calculate.

Below is a concise example that illustrates all three strategies:

```cpp
{
    auto a = var(1);
    auto b = calc([]() { return a(); });
    {
        auto temp = calc([]() { return a(); }); // default is DirectCloseStrategy
        b.set([]() { return temp(); });
    }
    // temp lifecycle ends, b will end too.
    EXPECT_FALSE(static_cast<bool>(b));
}
{
    auto a = var(1);
    auto b = calc([]() { return a(); });
    {
        auto temp = calc<AlwaysTrigger, KeepCalcStrategy>([]() { return a(); }); // default is DirectFailureStrategy
        b.set([]() { return temp(); });
    }
    // temp lifecycle ends, b not be influenced.
    EXPECT_TRUE(static_cast<bool>(b));
    EXPECT_EQ(b.get(), 1);
    a.value(2);
    EXPECT_EQ(b.get(), 2);
}
{
    auto a = var(1);
    auto b = calc([]() { return a(); });
    {
        auto temp = calc<AlwaysTrigger, KeepCalcStrategy>([]() { return a(); }); // default is DirectFailureStrategy
        b.set([]() { return temp(); });
    }
    // temp lifecycle ends, b use its last val to calculate.
    EXPECT_TRUE(static_cast<bool>(b));
    EXPECT_EQ(b.get(), 1);
    a.value(2);
    EXPECT_EQ(b.get(), 1);
}
```

Likewise, you can define a strategy yourself in your code, just include the **handleInvalid** method:

```cpp
struct MyStrategy {
    void handleInvalid() {
        std::cout << "Invalid" << std::endl;
    }
};
auto a = var(1);
auto b = expr<AlwaysTrigger, MyStrategy>(a + 1);
```
