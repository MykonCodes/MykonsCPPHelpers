// Copyright 2016 VRSpeaking, LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Function.h"
#include "ThreadSafeBool.h"
#include "SharedPointer.h"

/**
 * FCallbackSteward 
 * Helper class that hands out custom callbacks with dynamic parameter lists and notifies the user when all of them have been called via a callback.
 */
template<typename... CallbackArgs>
class OVATION_API FCallbackSteward : public TSharedFromThis<FCallbackSteward<CallbackArgs...>>
{
private:

	//Callback that will be fired when all acquired out-callbacks have been received
	TFunction<void(bool bSuccess)> OnFinished = nullptr;

	//Callback that will be called when one out-callback has been received, passes on the args and let's the user process the data.
	//return indicates whether it was interpreted as a success.
	TFunction<bool(CallbackArgs... args)> OnInstanceCalled = nullptr;

	//&='s all the returns of the callback's, will be passed on with OnFinished to let the user know if all out-callbacks have been successful. 
	bool bSuccessful = true;

	//Counts how many out-callbacks are pending
	int32 CallbackStackCount = 0;

	//Count how many callbacks have been handed out, to assign ID's
	int32 CallbackCounter = 0;

	//Whether the user marked the Steward as ready. Only when bReady = true, the steward can finish the operation;
	FThreadSafeBool bReady = false;

	//Keeps track of which callbacks have already been called to prevent double-called ones to throw the counter out of sync
	TArray<int32> CallbackHistory;

	//Whether incoming Cancel() can (not) self-destroy. Set to false (Can't selfdestruct) when Cancel() is called in the response to calling 'callback';
	FThreadSafeBool CallbackLock = false;

	//Cached the request to self destruct
	FThreadSafeBool bSelfDestruct = false;

	//Shared self, keeps object alive.
	TSharedPtr<FCallbackSteward> Self;

	//We do not want to alloc a steward on the stack as it self destructs and needs to performs async anyway.
	FCallbackSteward(TFunction<bool(CallbackArgs... args)> InCallback, TFunction<void(bool bSuccess)> InOnFinished)
		: OnInstanceCalled(InCallback)
		, OnFinished(InOnFinished)
	{}

public:

	//Generates a steward ptr. We do not want to alloc a steward on the stack as it self destructs and needs to performs async anyway.
	static FCallbackSteward* GenerateSteward(TFunction<bool(CallbackArgs... args)> InCallback, TFunction<void(bool bSuccess)> InOnFinished) {
		TSharedPtr<FCallbackSteward> NewSteward = MakeShareable(new FCallbackSteward(InCallback, InOnFinished));
		NewSteward->Self = NewSteward;
		return NewSteward.Get();
	}

	/**
	 * Indicates that all out-callbacks have been acquired and now when waitCount reaches 0, the Steward has done it's job.
	 * @return 
	 */
	bool SetReady() {

		//SetReady should never be called within a callback called by this steward. 
		ensureAlways(!CallbackLock);

		bReady = true;
		if (CallbackStackCount <= 0) {
			OnFinished(bSuccessful);

			//Callback called, self destruct
			Self.Reset();

			//Yes, we are finished already
			return true;
		}

		//No, we're not finished already.
		return false;
	}

	//Destroy self
	void Cancel() {
		//If we're locked, set SelfDestruct to true instead
		if (!CallbackLock) {
			Self.Reset();
		}
		else {
			bSelfDestruct.AtomicSet(true);
		}
	}

	TFunction<void(CallbackArgs... args)> AcquireCallback(TFunction<bool(CallbackArgs...)> CustomCallback = nullptr) {

		//We never should acquire new callbacks after setting the steward to be ready;
		if (bReady) {
			ensureAlways(false);
			return nullptr;
		}

		//We now want to wait for one more callback to be called
		++CallbackStackCount;

		//And we handed one more out. We can't use the same integers for this, as otherwise ID's could be reused by callbacks that finish before others are handed out.
		++CallbackCounter;

		//Copy and capture the CallbackCounter by copy as the ID for the callback. Used to make sure that each callback only reduces the CallbackStackCount the first time it is called.
		int32 MyCallbackID = CallbackCounter;

		//Capture shared self to prevent deletion while callbacks may still be incoming
		TSharedPtr<FCallbackSteward> SharedSelf = AsShared();

		return [this, SharedSelf, MyCallbackID, CustomCallback](CallbackArgs... args) {

			if (bSelfDestruct) return;


			//We received an out callback, decrement wait counter, if this callback hasn't already been called.
			if (!CallbackHistory.Contains(MyCallbackID)) {
				--CallbackStackCount;
				CallbackHistory.Add(MyCallbackID);
			}

			//Enable callback lock that prevents self destruction as a result of calling the callback
			CallbackLock.AtomicSet(true);

			//Pass on callback to let the user do whatever he wants and be the judge if it was successful. 
			bSuccessful &= CustomCallback ? CustomCallback(args...) : OnInstanceCalled(args...);

			//Remove callback lock again
			CallbackLock.AtomicSet(false);

			//If marked for destruction, self destroy
			if (bSelfDestruct) {
				Self.Reset();
			}
			//Now, if all callbacks have been initialized (bReady) and received (waitCount <= 0), fire OnFinished
			else if (bReady && CallbackStackCount <= 0) {
				OnFinished(bSuccessful);

				//Callback called, self destruct
				Self.Reset();
			}
		};
	}
};