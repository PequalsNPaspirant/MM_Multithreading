#include "Multithreading/SemaphoreUsingConditionVariable.h"

namespace mm {

	namespace SemaphoreUsingConditionVariable {
		

	}

	MM_DECLARE_FLAG(SemaphoreUsingConditionVariable_v1);

	MM_UNIT_TEST(SemaphoreUsingConditionVariable_v1_Test, SemaphoreUsingConditionVariable_v1)
	{
		SemaphoreUsingConditionVariable::FixedSizeThreadSafeQueue fixedSizeQueue{ 100 };
		SemaphoreUsingConditionVariable::usage(fixedSizeQueue);
		
		SemaphoreUsingConditionVariable::UnlimitedSizeThreadSafeQueue unlimitedSizeQueue{};
		SemaphoreUsingConditionVariable::usage(unlimitedSizeQueue);
	}

}

