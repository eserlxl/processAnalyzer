# Testing

The project uses **GoogleTest** for unit testing. Tests are automatically discovered and built when you follow the standard build instructions.

## Running the Test Suite

After building the project, you can run tests in two ways:

1.  **Run all tests with CTest**:
    This is the recommended way to run the entire test suite. From your `build` directory, execute:
    ```bash
    ctest --output-on-failure
    ```

2.  **Run the test executable directly**:
    This method allows for more granular control, such as running specific test cases or using GoogleTest flags. The single test executable is located at `build/tests/ProcessAnalyzerTests`.

    ```bash
    # Run all tests directly
    ./build/tests/ProcessAnalyzerTests

    # Example: Run only tests related to the Core library
    ./build/tests/ProcessAnalyzerTests --gtest_filter="Core*"
    ```

For more detailed information on the build process and testing, see the [Build Details](../build.md) document.
