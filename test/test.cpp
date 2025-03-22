// test/test.cpp
#include "gtest/gtest.h"
#include "reaction/dataSource.h"
#include <iostream>

TEST(TestConstructor, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    EXPECT_EQ(a->get(), 1);
    EXPECT_EQ(b->get(), 3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb)
                                               { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds)
                                                { return std::to_string(aa) + dsds; }, a, ds);
    EXPECT_EQ(ds->get(), "13.140000");
    EXPECT_EQ(dds->get(), "113.140000");
}

TEST(TestCommonUse, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb)
                                               { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds)
                                                { return std::to_string(aa) + dsds; }, a, ds);
    *a = 2;
    EXPECT_EQ(ds->get(), "23.140000");
    EXPECT_EQ(dds->get(), "223.140000");
}

TEST(TestConstDataSource, ReactionTest)
{
    auto a = reaction::makeConstantDataSource(1);
    auto b = reaction::makeConstantDataSource(3.14);
    // *a = 2;
    auto ds = reaction::makeVariableDataSource([](int aa, double bb)
                                               { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds)
                                                { return std::to_string(aa) + dsds; }, a, ds);
}

TEST(TestAction, ActionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](auto aa, auto bb)
                                               { return std::to_string(aa) + std::to_string(bb); }, a, b);
    int val = 10;
    auto dds = reaction::makeActionSource([&val](auto aa, auto dsds)
                                          { val = aa; }, a, ds);
    EXPECT_EQ(val, 1);
    *a = 2;
    EXPECT_EQ(val, 2);
}

TEST(TestReset, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(std::string{"2"});
    auto c = reaction::makeMetaDataSource(std::string{"3"});
    auto d = reaction::makeMetaDataSource(std::string{"4"});
    auto ds = reaction::makeVariableDataSource([](auto aa)
                                               { return std::to_string(aa); }, a);
    auto dds = reaction::makeVariableDataSource([](auto bb)
                                                { return bb; }, b);
    auto ddds = reaction::makeVariableDataSource([](auto cc)
                                                 { return cc; }, c);
    EXPECT_EQ(ddds->get(), "3");
    ddds->set([](auto dd, auto dsds)
              { return dd + dsds + "set"; }, d, dds);
    EXPECT_EQ(ddds->get(), "42set");
    *c = "33";
    EXPECT_EQ(ddds->get(), "42set");
    *d = "44";
    EXPECT_EQ(ddds->get(), "442set");
}

TEST(TestSelfDependency, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(2);
    auto c = reaction::makeMetaDataSource(3);

    auto dsA = reaction::makeVariableDataSource([](int aa)
                                                { return aa; }, a);

    EXPECT_EQ(dsA->set([](int aa, int dsAValue)
                       { return aa + dsAValue; }, a, dsA),
              false);
}

TEST(TestRepeatDependency, ReactionTest) {
    auto a = reaction::makeMetaDataSource(1);

    // 创建数据源 dsA，依赖 a 和 b
    auto dsA = reaction::makeVariableDataSource([](int aa) {
        return aa;
    }, a);

    // 创建数据源 dsB，依赖 a、b 和 dsA
    auto dsB = reaction::makeVariableDataSource([](int aa, int dsAValue) {
        return aa + dsAValue;
    }, a, dsA);

    // 创建数据源 dsC，依赖 b、c 和 dsB
    auto dsC = reaction::makeVariableDataSource([](int aa, int dsAValue, int dsBValue) {
        return aa + dsAValue + dsBValue;
    }, a, dsA, dsB);

    // 创建数据源 dsD，依赖 c、d 和 dsC
    auto dsD = reaction::makeVariableDataSource([](int dsAValue, int dsBValue, int dsCValue) {
        return dsAValue + dsBValue + dsCValue;
    }, dsA, dsB, dsC);


    // 创建数据源 dsF，依赖 dsA、dsB 和 dsE
    auto dsE = reaction::makeVariableDataSource([](int dsBValue, int dsCValue, int dsDValue) {
        return dsBValue * dsCValue + dsDValue;
    }, dsB, dsC, dsD);

    // 检查初始值
    EXPECT_EQ(dsA->get(), 1);   // 1 + 2 = 3
    EXPECT_EQ(dsB->get(), 2);   // 1 * 2 + 3 = 5
    EXPECT_EQ(dsC->get(), 4);   // 2 - 3 + 5 = 4
    EXPECT_EQ(dsD->get(), 7);   // 3 / 4 + 4 ≈ 4 (整数除法)
    EXPECT_EQ(dsE->get(), 15);  // 4 + 5 + 4 = 13

    // 修改 a 的值，检查所有数据源是否更新
    *a = 10;
    EXPECT_EQ(dsA->get(), 10);   // 1 + 2 = 3
    EXPECT_EQ(dsB->get(), 20);   // 1 * 2 + 3 = 5
    EXPECT_EQ(dsC->get(), 40);   // 2 - 3 + 5 = 4
    EXPECT_EQ(dsD->get(), 70);   // 3 / 4 + 4 ≈ 4 (整数除法)
    EXPECT_EQ(dsE->get(), 150);  // 4 + 5 + 4 = 13

}

TEST(TestCycleDependency, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(2);
    auto c = reaction::makeMetaDataSource(3);

    auto dsA = reaction::makeVariableDataSource([](int bb)
                                                { return bb; }, b);

    auto dsB = reaction::makeVariableDataSource([](int cc)
                                                { return cc; }, c);

    auto dsC = reaction::makeVariableDataSource([](int aa)
                                                { return aa; }, a);

    dsA->set([](int bb, int dsBValue)
             { return bb + dsBValue; }, b, dsB);

    dsB->set([](int cc, int dsCValue)
             { return cc * dsCValue; }, c, dsC);

    EXPECT_EQ(dsC->set([](int aa, int dsAValue)
                       { return aa - dsAValue; }, a, dsA),
              false);
}



int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}