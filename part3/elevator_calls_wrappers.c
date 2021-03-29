#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
//needed in order to compile

//Declares function pointers for syscalls and exports them for module usage
long (*STUB_start_elevator)(void) = NULL;
long (*STUB_issue_request)(int, int, int) = NULL;
long (*STUB_stop_elevator)(void) = NULL;
EXPORT_SYMBOL_GPL(STUB_start_elevator);
EXPORT_SYMBOL_GPL(STUB_issue_request);
EXPORT_SYMBOL_GPL(STUB_stop_elevator);

//Start elevator wrapper
SYSCALL_DEFINE0(start_elevator)
{
  if (STUB_start_elevator != NULL)
    return STUB_start_elevator();
  else
    return -ENOSYS;
}

//Issue request wrapper
SYSCALL_DEFINE3(issue_request, int ,passengerCarryType, int ,currentFloor, int ,destinationFloor)
{
  if (STUB_issue_request != NULL)
    return STUB_issue_request(passengerCarryType, currentFloor, destinationFloor);
  else
    return -ENOSYS;
}

//Stop elevator wrapper
SYSCALL_DEFINE0(stop_elevator)
{
  if (STUB_stop_elevator != NULL)
    return STUB_stop_elevator();
  else
    return -ENOSYS;
}
