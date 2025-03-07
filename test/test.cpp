// test/test.cpp
#include "gtest/gtest.h"
#include "reaction/dataSource.h"
#include <iostream>

// 循环依赖问题done
// 重复依赖问题done
// 数据源重绑定done
// 访问权限问题done
// 用户异常提示done
// 数据源失效问题done
// valueCache功能done
// actionSource功能done
// 内存管理问题done
// 多线程支持
// cornercase检查
// 合并traits

TEST(TestConstructor, ReactionTest) {
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    EXPECT_EQ(a->get(), 1);
    EXPECT_EQ(b->get(), 3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb) {
        return std::to_string(aa) + std::to_string(bb);
    }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds) {
        return std::to_string(aa) + dsds;
    }, a, ds);
    EXPECT_EQ(ds->get(), "13.140000");
    EXPECT_EQ(dds->get(), "113.140000");
}

TEST(TestCommonUse, ReactionTest) {
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb) {
        return std::to_string(aa) + std::to_string(bb);
    }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds) {
        return std::to_string(aa) + dsds;
    }, a, ds);
    *a = 2;
    EXPECT_EQ(ds->get(), "23.140000");
    EXPECT_EQ(dds->get(), "223.140000");
}

TEST(TestConstDataSource, ReactionTest) {
    auto a = reaction::makeConstantDataSource(1);
    auto b = reaction::makeConstantDataSource(3.14);
    // *a = 2;
    auto ds = reaction::makeVariableDataSource([](int aa, double bb) {
        return std::to_string(aa) + std::to_string(bb);
    }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds) {
        return std::to_string(aa) + dsds;
    }, a, ds);
}

TEST(TestAction, ActionTest) {
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](auto aa, auto bb) {
        return std::to_string(aa) + std::to_string(bb);
    }, a, b);
    int val = 10;
    auto dds = reaction::makeActionSource([&val](auto aa, auto dsds) {
        val = aa;
    }, a, ds);
    EXPECT_EQ(val, 1);
    *a = 2;
    EXPECT_EQ(val, 2);
}

TEST(TestReset, ReactionTest) {
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(std::string{"2"});
    auto c = reaction::makeMetaDataSource(std::string{"3"});
    auto d = reaction::makeMetaDataSource(std::string{"4"});
    auto ds = reaction::makeVariableDataSource([](auto aa) {
        return std::to_string(aa);
    }, a);
    auto dds = reaction::makeVariableDataSource([](auto bb) {
        return bb;
    }, b);
    auto ddds = reaction::makeVariableDataSource([](auto cc) {
        return cc;
    }, c);
    EXPECT_EQ(ddds->get(), "3");
    ddds->set([](auto dd, auto dsds) {
        return dd + dsds + "set";
    }, d, dds);
    EXPECT_EQ(ddds->get(), "42set");
    *c = "33";
    EXPECT_EQ(ddds->get(), "42set");
    *d = "44";
    EXPECT_EQ(ddds->get(), "442set");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}