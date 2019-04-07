//=======================================================================================================//
//   Copyright (c) 2018 Maruti Mhetre                                                                    //
//   All rights reserved.                                                                                //
//=======================================================================================================//
//   Redistribution and use of this software in source and binary forms, with or without modification,   //
//   are permitted for personal, educational or non-commercial purposes provided that the following      //
//   conditions are met:                                                                                 //
//   1. Redistributions of source code must retain the above copyright notice, this list of conditions   //
//      and the following disclaimer.                                                                    //
//   2. Redistributions in binary form must reproduce the above copyright notice, this list of           //
//      conditions and the following disclaimer in the documentation and/or other materials provided     //
//      with the distribution.                                                                           //
//   3. Neither the name of the copyright holder nor the names of its contributors may be used to        //
//      endorse or promote products derived from this software without specific prior written            //
//      permission.                                                                                      //
//=======================================================================================================//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR      //
//   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND    //
//   FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR          //
//   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL   //
//   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   //
//   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER  //
//   IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT   //
//   OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                     //
//=======================================================================================================//

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono>
using namespace std;

namespace mm {

// If you want to redirect the output to any other output destination (such as file)
// OR if you want to print something fancy (like printing Ok in green and FAILED in 
// red on screen), overwrite macro MM_COUT
// Make sure you have operator<< overridden for your type. Because it is called as:  MM_COUT << some_output;
#define MM_COUT std::cout

//---- Macro version 4 ---	
/*
This macro can be called as:
__PRINT_VARIABLES_AND_VALUES__(amount1 == amount2 && amount2 != amount3, amount1, amount2, amount3)

Output:

*/

	enum class Mode
	{
		read = 0,
		write = 1
	};

	class dummyType {};

#define MM_PRINT_TEST_CASE_NUMBER(flag) printTestCaseNumber<dummyType>(Mode::write, flag);

	template<typename Type>
	bool printTestCaseNumber(Mode mode = Mode::read, bool flag = true)
	{
		static bool printTestCaseNumberValue = flag;
		if (mode == Mode::write)
			printTestCaseNumberValue = flag;
		return printTestCaseNumberValue;
	}

#define MM_PRINT_TEST_RESULTS_ON_SINGLE_LINE(flag) printTestResultsOnSingleLine<dummyType>(Mode::write, flag);

	template<typename Type>
	bool printTestResultsOnSingleLine(Mode mode = Mode::read, bool flag = false)
	{
		static bool printTestResultsOnSingleLineValue = flag;
		if (mode == Mode::write)
			printTestResultsOnSingleLineValue = flag;
		return printTestResultsOnSingleLineValue;
	}

#define MM_SET_PAUSE_ON_ERROR(flag) mm::pauseOnError<dummyType>(Mode::write, flag);

	template<typename Type>
	bool pauseOnError(Mode mode = Mode::read, bool flag = true)
	{
		static bool pauseOnErrorValue = flag;
		if (mode == Mode::write)
			pauseOnErrorValue = flag;
		return pauseOnErrorValue;
	}

	// recursive variadic function
	template<typename... ZeroOrManyTypes>
	void printDifferentTypesOfVariables(size_t width, string str, ZeroOrManyTypes... args)  // Handle zero variables
	{
	}

	template <typename ExactlyOneType>
	void printDifferentTypesOfVariables(size_t width, string str, ExactlyOneType t)  // Handle exactly one variables
	{
		if(printTestResultsOnSingleLine<dummyType>())
			MM_COUT << " ||| ";
		else
			MM_COUT << "\n";
		MM_COUT << std::setw(width) << str << " : " << t;
	}

	template<typename FirstType, typename... RestTypes>
	void printDifferentTypesOfVariables(size_t width, string str, FirstType t, RestTypes... args)  // used when at least one variable is provided
	{
		int firstCommaPosition = str.find_first_of(',');
		printDifferentTypesOfVariables(width, str.substr(0, firstCommaPosition), t);
		//MM_COUT << "\n";
		printDifferentTypesOfVariables(width, str.substr(firstCommaPosition + 1, string::npos), args...);
	}

#define __PRINT_VARIABLES_AND_VALUES__4(separator, validationStatement, ...) \
	string variables = string(#__VA_ARGS__); \
	if (!variables.empty()) \
	{ \
		size_t width = 0; \
		size_t initialSpacing = 5; \
		size_t prevPos = 0; \
		size_t nextPos = 0; \
		while ((nextPos = variables.find(',', prevPos + 1)) != string::npos) \
		{ \
			if (width < (nextPos - prevPos + 1)) width = nextPos - prevPos + 1; \
			prevPos = nextPos; \
		} \
		width += initialSpacing; \
		MM_COUT << separator << setw(width + 1) << "Variables" << " : " << "Values"; \
		cout << std::boolalpha; \
		mm::printDifferentTypesOfVariables(width + separator.length(), variables, ##__VA_ARGS__); \
	}

//---- END of Macro versions ---

	typedef void(*__testCaseFunPtr__)();

	//----------------- Global container to store unit test function pointers ----------------------
	template<typename dataType>
	std::vector<__testCaseFunPtr__>& getGlobalTestCasesVector()
	{
		static std::vector<__testCaseFunPtr__> globalTestCases;
		return globalTestCases;
	}

	template<typename dataType>
	dataType& getTestCaseCounter()
	{
		static dataType counter = 0;
		return counter;
	}

#define __MM_DECLARE_FUNCTION__(testcase) void testcase();
#define __MM_ADD_TEST_CASE__(testcase) mm::getGlobalTestCasesVector<dummyType>().push_back(testcase);

	template<typename dummyDataType>
	string getCommaSeparatedTimeDuration(unsigned long long duration)
	{
		string durationStr = "000,000.000,000,000"; //Time less than 1,000,000 sec can be measured with this
		int pos = durationStr.length() - 1;
		while (duration > 0 && --pos > 0)
		{
			if (durationStr[pos] == '0')
			{
				durationStr[pos] = '0' + duration % 10;
				duration /= 10;
			}
		}
		if (pos > 6)
			pos = 6;
		durationStr = durationStr.substr(pos);
		durationStr += " sec";

		return durationStr;
	}

#define __MM_EXPECT_TRUE_PRINT__(printAllResults, measureTime, validationStatement, ...) \
	{ \
		int lineNumber = __LINE__; \
		string fileName = __FILE__; \
		std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now(); \
		bool __mm_unit_test_result__ = validationStatement; \
		std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now(); \
		unsigned long long time = 0; \
		if(measureTime) \
			time = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count(); \
		string resultStr("NOT OK"); \
		if (__mm_unit_test_result__ == true) \
		{ \
			resultStr = "OK"; \
		} \
		/*Do all printing stuff here*/ \
		string separator("\n      ===>"); \
		bool resultsOnSingleLine = printTestResultsOnSingleLine<dummyType>(); \
		if(resultsOnSingleLine) \
			separator = " ||| "; \
		if(printTestCaseNumber<dummyType>()) \
		{ \
			if(printAllResults || measureTime) /*Need to print test case no. on new line*/ \
				MM_COUT << "\n\n"; \
			else \
				MM_COUT << "\rExecuting "; \
			MM_COUT << "Test Case No. : " << ++getTestCaseCounter<size_t>(); \
		} \
		if(printAllResults || !__mm_unit_test_result__) \
		{ \
			MM_COUT << separator << " TEST_RESULT          : " << resultStr; \
		} \
		if(measureTime) \
		{ \
			constexpr const int columnWidth = 21; \
			MM_COUT << separator << " TIME                 : " << setw(columnWidth) << std::right << getCommaSeparatedTimeDuration<dummyType>(time); \
		} \
		/*Print results in case of failures*/ \
		if(!__mm_unit_test_result__ || measureTime || printAllResults) \
		{ \
			MM_COUT << separator << " Function             : " << string(__FUNCTION__); \
			MM_COUT << separator << " File                 : " << fileName.substr(fileName.find_last_of('\\') + 1, string::npos); \
			MM_COUT << separator << " Line                 : " << lineNumber; \
			MM_COUT << separator << " Validation Statement : " << string(#validationStatement); \
			__PRINT_VARIABLES_AND_VALUES__4(separator, validationStatement, ##__VA_ARGS__) \
			if(!__mm_unit_test_result__ && pauseOnError<dummyType>()) \
			{ \
				MM_COUT << "\nPress any key to continue..."; \
				cin.get(); \
			} \
		} \
	}

#define MM_EXPECT_TRUE(validationStatement, ...) __MM_EXPECT_TRUE_PRINT__(false, false, validationStatement, ##__VA_ARGS__)
#define MM_EXPECT_TRUE_PRINTALLRESULTS(validationStatement, ...) __MM_EXPECT_TRUE_PRINT__(true, false, validationStatement, ##__VA_ARGS__)
#define MM_TIMED_EXPECT_TRUE(validationStatement, ...) __MM_EXPECT_TRUE_PRINT__(true, true, validationStatement, ##__VA_ARGS__)

#define MM_DECLARE_FLAG(flagName) \
	bool mm_get_##flagName();

#define MM_DEFINE_FLAG(result, flagName) \
	bool mm_get_##flagName() \
	{ \
		return result; \
	}

#define __MM_UNIT_TEST__(functionName, flagName) \
	__MM_DECLARE_FUNCTION__(functionName) \
	class mm_class_##functionName \
	{ \
	public: \
		mm_class_##functionName() \
		{ \
			if(mm_get_##flagName()) \
				__MM_ADD_TEST_CASE__(functionName) \
		} \
	}; \
	mm_class_##functionName mm_global_object_##functionName; \
	void functionName()

	//----------------- this function is called from main() using the macro defined underline ----------------------
	template<typename dataType>
	void runTests()
	{
		MM_COUT << "\n\nExecuting unit tests...\n\n";
		size_t totalTests = getGlobalTestCasesVector<dummyType>().size();
		size_t counter = 0;
		for (__testCaseFunPtr__ funptr : getGlobalTestCasesVector<dummyType>())
		{
			(*funptr)();
		}
		MM_COUT << "\n\nExecution of unit tests is finished. All tests have passed unless any failure printed above!";
	}

#define MM_UNIT_TEST(functionName, flagName) \
	MM_DECLARE_FLAG(flagName) \
	__MM_UNIT_TEST__(functionName, flagName)

#define MM_RUN_UNIT_TESTS mm::runTests<dummyType>();





	//========================= below code is absolete ====================================
#if 0
	//---- Macro version 1 ---

	// This macro can be called as:
	//__PRINT_VARIABLES_AND_VALUES__1(amount1 == amount2 && amount2 != amount3, "%d %d %d", amount1, amount2, amount3)

#define __PRINT_VARIABLES_AND_VALUES__1(validationStatement, formatString, ...) \
	MM_COUT << "\n    Validation Statement: " << string(#validationStatement); \
	MM_COUT << "\n    Values: "; \
	printf(formatString, ##__VA_ARGS__);

	//---- Macro version 2 ---
	/*
	This macro can be called as:
	__PRINT_VARIABLES_AND_VALUES__2(amount1 == amount2 && amount2 != amount3, amount1, amount2, amount3)

	Output:
	Validation Statement: amount1 == amount2 && amount2 != amount3
	Variables: amount1, amount2, amount3
	Values   : 2, 2, 2,
	*/

	template <class T>
	void printInitializerList(std::initializer_list<T> list)
	{
		for (auto elem : list)
			MM_COUT << elem << ", ";
	}

#define __PRINT_VARIABLES_AND_VALUES__2(validationStatement, ...) \
	MM_COUT << "\n    Validation Statement: " << string(#validationStatement); \
	MM_COUT << "\n    Variables: " << string(#__VA_ARGS__); \
	MM_COUT << "\n    Values   : "; \
	printInitializerList({##__VA_ARGS__});

	//---- Macro version 3 ---

	/*
	This macro can be called as:
	__PRINT_VARIABLES_AND_VALUES__(amount1 == amount2 && amount2 != amount3, amount1, amount2, amount3)

	Output:
	Validation Statement: amount1 == amount2 && amount2 != amount3
	Variables: amount1, amount2, amount3
	Values   : 2, 2, 2
	*/

	// recursive variadic function
	template<typename... ZeroOrManyTypes>
	void printDifferentTypesOfVariables(ZeroOrManyTypes... args)  // Handle zero variables
	{
	}

	template <typename ExactlyOneType>
	void printDifferentTypesOfVariables(ExactlyOneType t)  // Handle exactly one variables
	{
		MM_COUT << t;
	}

	template<typename FirstType, typename... RestTypes>
	void printDifferentTypesOfVariables(FirstType t, RestTypes... args)  // used when at least one variable is provided
	{
		printDifferentTypesOfVariables(t);
		MM_COUT << ", ";
		printDifferentTypesOfVariables(args...);
	}

#define __PRINT_VARIABLES_AND_VALUES__3(validationStatement, ...) \
	MM_COUT << "\n    Validation Statement: " << string(#validationStatement); \
	if(string(#__VA_ARGS__) != "") \
	{ \
		MM_COUT << "\n    Variables: " << string(#__VA_ARGS__); \
		cout << std::boolalpha; \
		MM_COUT << "\n    Values   : "; \
		mm::printDifferentTypesOfVariables(##__VA_ARGS__); \
		MM_COUT << "\n"; \
	}

	//---- END of Macro versions ---
#endif

}
