#include "Multithreading/SemaphoreUsingConditionVariable.h"

namespace mm {

	namespace SemaphoreUsingConditionVariable {
		

	}

	MM_DECLARE_FLAG(SemaphoreUsingConditionVariable);

	MM_UNIT_TEST(SemaphoreUsingConditionVariable_Test, SemaphoreUsingConditionVariable)
	{
		SemaphoreUsingConditionVariable::FixedSizeThreadSafeQueue fixedSizeQueue{ 100 };
		SemaphoreUsingConditionVariable::usage(fixedSizeQueue);
		
		SemaphoreUsingConditionVariable::UnlimitedSizeThreadSafeQueue unlimitedSizeQueue{};
		SemaphoreUsingConditionVariable::usage(unlimitedSizeQueue);
	}

}

