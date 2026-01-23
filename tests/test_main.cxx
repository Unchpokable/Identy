#include <gtest/gtest.h>
#include <iostream>

#include "test_config.hxx"

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    std::cout << "========================================\n";
    std::cout << "Identy Library Test Suite\n";
    std::cout << "========================================\n";
    std::cout << "Expected environment: " << identy::test::GetExpectedEnvironmentDescription() << "\n";
    std::cout << "Environment explicitly set: " << (identy::test::kEnvironmentExplicitlySet ? "Yes" : "No (auto-detected)") << "\n";
    std::cout << "========================================\n\n";

    return RUN_ALL_TESTS();
}
