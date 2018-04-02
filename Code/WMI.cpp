#include "Precompiled.h"
#include <wbemidl.h>
#include <comdef.h>
#include "WMI.h"
#include <Powersetting.h>

// You can choose between setting the brightness directly ("simple" method with WMI) or the method that sets the power plan. WMI was default for monitors having no DDC/CI until now.
//#define USE_WMI

#if USE_WMI
	// http://stexbar.googlecode.com/svn-history/r264/trunk/Misc/SKBright/src/Utils.cpp
	// PS C : \Users\florian > Get - Ciminstance - Namespace root / WMI - ClassName WmiMonitorBrightness
	//
	//
	// Active : True
	//	CurrentBrightness : 70
	//	InstanceName : DISPLAY\SDC3853\4 & 3b1d693f & 3 & UID265988_0
	//	Level : {1, 2, 3, 4...}
	// Levels: 100
	//	PSComputerName :
	bool Wmi::GetBrightnessInfo(int *brightness, int *levels) {
		bool ret = false;

		IWbemLocator *pLocator = NULL;
		IWbemServices *pNamespace = 0;
		IEnumWbemClassObject *pEnum = NULL;
		HRESULT hr = S_OK;

		BSTR path = SysAllocString(L"root\\wmi");
		BSTR ClassPath = SysAllocString(L"WmiMonitorBrightness");
		BSTR bstrQuery = SysAllocString(L"Select * from WmiMonitorBrightness");

		if (!path || !ClassPath) {
			goto cleanup;
		}


		// Initialize COM and connect up to CIMOM
		hr = CoInitialize(0);
		if (FAILED(hr)) {
			goto cleanup;
		}

		//  NOTE:
		//  When using asynchronous WMI API's remotely in an environment where the "Local System" account
		//  has no network identity (such as non-Kerberos domains), the authentication level of
		//  RPC_C_AUTHN_LEVEL_NONE is needed. However, lowering the authentication level to
		//  RPC_C_AUTHN_LEVEL_NONE makes your application less secure. It is wise to
		// use semi-synchronous API's for accessing WMI data and events instead of the asynchronous ones.
		hr = CoInitializeSecurity ( NULL, -1, NULL, NULL,
			RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL,
			EOAC_SECURE_REFS, //change to EOAC_NONE if you change dwAuthnLevel to RPC_C_AUTHN_LEVEL_NONE
			NULL );

		hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID *) &pLocator);
		if (FAILED(hr)) {
			goto cleanup;
		}
		hr = pLocator->ConnectServer(path, NULL, NULL, NULL, 0, NULL, NULL, &pNamespace);
		if (hr != WBEM_S_NO_ERROR) {
			goto cleanup;
		}

		hr = CoSetProxyBlanket(pNamespace,
			RPC_C_AUTHN_WINNT,
			RPC_C_AUTHZ_NONE,
			NULL,
			RPC_C_AUTHN_LEVEL_PKT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL,
			EOAC_NONE
			);

		if (hr != WBEM_S_NO_ERROR) {
			goto cleanup;
		}


		hr = pNamespace->ExecQuery(_bstr_t(L"WQL"), //Query Language
			bstrQuery, //Query to Execute
			WBEM_FLAG_RETURN_IMMEDIATELY, //Make a semi-synchronous call
			NULL, //Context
			&pEnum //Enumeration Interface
			);

		if (hr != WBEM_S_NO_ERROR) {
			goto cleanup;
		}

		hr = WBEM_S_NO_ERROR;

		while (WBEM_S_NO_ERROR == hr) {

			ULONG ulReturned;
			IWbemClassObject *pObj;
			DWORD retVal = 0;

			//Get the Next Object from the collection
			hr = pEnum->Next(WBEM_INFINITE, //Timeout
				1, //No of objects requested
				&pObj, //Returned Object
				&ulReturned //No of object returned
				);

			if (hr != WBEM_S_NO_ERROR) {
				goto cleanup;
			}

			VARIANT var1;
			hr = pObj->Get(_bstr_t(L"CurrentBrightness"),0,&var1,NULL,NULL);
			*brightness = V_UI1(&var1);
			VariantClear(&var1);
			if (hr != WBEM_S_NO_ERROR) {
				goto cleanup;
			}

			hr = pObj->Get(_bstr_t(L"Levels"),0,&var1,NULL,NULL);
			*levels = V_UI1(&var1);
			VariantClear(&var1);
			if (hr != WBEM_S_NO_ERROR) {
				goto cleanup;
			}
			ret = true;
		}

		// Free up resources
	cleanup:

		SysFreeString(path);
		SysFreeString(ClassPath);
		SysFreeString(bstrQuery);

		if (pLocator)
			pLocator->Release();
		if (pNamespace)
			pNamespace->Release();

		CoUninitialize();

		return ret;
	}

	bool Wmi::SetBrightness(int val) {
		bool bRet = true;

		IWbemLocator *pLocator = NULL;
		IWbemServices *pNamespace = 0;
		IWbemClassObject * pClass = NULL;
		IWbemClassObject * pInClass = NULL;
		IWbemClassObject * pInInst = NULL;
		IEnumWbemClassObject *pEnum = NULL;
		HRESULT hr = S_OK;

		BSTR path = SysAllocString(L"root\\wmi");
		BSTR ClassPath = SysAllocString(L"WmiMonitorBrightnessMethods");
		BSTR MethodName = SysAllocString(L"WmiSetBrightness");
		BSTR ArgName0 = SysAllocString(L"Timeout");
		BSTR ArgName1 = SysAllocString(L"Brightness");
		BSTR bstrQuery = SysAllocString(L"Select * from WmiMonitorBrightnessMethods");

		if (!path || ! ClassPath || !MethodName || ! ArgName0) {
			bRet = false;
			goto cleanup;
		}


		// Initialize COM and connect up to CIMOM

		hr = CoInitialize(0);
		if (FAILED(hr)) {
			bRet = false;
			goto cleanup;
		}

		//  NOTE:
		//  When using asynchronous WMI API's remotely in an environment where the "Local System" account
		//  has no network identity (such as non-Kerberos domains), the authentication level of
		//  RPC_C_AUTHN_LEVEL_NONE is needed. However, lowering the authentication level to
		//  RPC_C_AUTHN_LEVEL_NONE makes your application less secure. It is wise to
		// use semi-synchronous API's for accessing WMI data and events instead of the asynchronous ones.

		hr = CoInitializeSecurity ( NULL, -1, NULL, NULL,
			RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL,
			EOAC_SECURE_REFS, //change to EOAC_NONE if you change dwAuthnLevel to RPC_C_AUTHN_LEVEL_NONE
			NULL );

		hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID *) &pLocator);
		if (FAILED(hr)) {
			bRet = false;
			goto cleanup;
		}
		hr = pLocator->ConnectServer(path, NULL, NULL, NULL, 0, NULL, NULL, &pNamespace);
		if (hr != WBEM_S_NO_ERROR) {
			bRet = false;
			goto cleanup;
		}


		hr = CoSetProxyBlanket(pNamespace,
			RPC_C_AUTHN_WINNT,
			RPC_C_AUTHZ_NONE,
			NULL,
			RPC_C_AUTHN_LEVEL_PKT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL,
			EOAC_NONE
			);

		if (hr != WBEM_S_NO_ERROR) {
			bRet = false;
			goto cleanup;
		}


		hr =pNamespace->ExecQuery(_bstr_t(L"WQL"), //Query Language
			bstrQuery, //Query to Execute
			WBEM_FLAG_RETURN_IMMEDIATELY, //Make a semi-synchronous call
			NULL, //Context
			&pEnum //Enumeration Interface
			);

		if (hr != WBEM_S_NO_ERROR) {
			bRet = false;
			goto cleanup;
		}

		hr = WBEM_S_NO_ERROR;

		while (WBEM_S_NO_ERROR == hr) {

			ULONG ulReturned;
			IWbemClassObject *pObj;
			DWORD retVal = 0;

			//Get the Next Object from the collection
			hr = pEnum->Next(WBEM_INFINITE, //Timeout
				1, //No of objects requested
				&pObj, //Returned Object
				&ulReturned //No of object returned
				);

			if (hr != WBEM_S_NO_ERROR) {
				bRet = false;
				goto cleanup;
			}

			// Get the class object
			hr = pNamespace->GetObject(ClassPath, 0, NULL, &pClass, NULL);
			if (hr != WBEM_S_NO_ERROR) {
				bRet = false;
				goto cleanup;
			}

			// Get the input argument and set the property
			hr = pClass->GetMethod(MethodName, 0, &pInClass, NULL);
			if (hr != WBEM_S_NO_ERROR) {
				bRet = false;
				goto cleanup;
			}

			hr = pInClass->SpawnInstance(0, &pInInst);
			if (hr != WBEM_S_NO_ERROR) {
				bRet = false;
				goto cleanup;
			}

			VARIANT var1;
			VariantInit(&var1);

			V_VT(&var1) = VT_BSTR;
			V_BSTR(&var1) = SysAllocString(L"0");
			hr = pInInst->Put(ArgName0, 0, &var1, CIM_UINT32); //CIM_UINT64

			//var1.vt = VT_I4;
			//var1.ullVal = 0;
			//   hr = pInInst->Put(ArgName0, 0, &var1, 0);
			VariantClear(&var1);
			if (hr != WBEM_S_NO_ERROR) {
				bRet = false;
				goto cleanup;
			}

			VARIANT var;
			VariantInit(&var);

			V_VT(&var) = VT_BSTR;
			WCHAR buf[10]={0};
			wsprintfW(buf, L"%ld", val);
			V_BSTR(&var) = SysAllocString(buf);
			hr = pInInst->Put(ArgName1, 0, &var, CIM_UINT8);

			//var.vt=VT_UI1;
			//var.uiVal = 100;
			//hr = pInInst->Put(ArgName1, 0, &var, 0);
			VariantClear(&var);
			if (hr != WBEM_S_NO_ERROR) {
				bRet = false;
				goto cleanup;
			}
			// Call the method

			VARIANT pathVariable;
			VariantInit(&pathVariable);

			hr = pObj->Get(_bstr_t(L"__PATH"),0,&pathVariable,NULL,NULL);
			if (hr != WBEM_S_NO_ERROR)
				goto cleanup;

			hr =pNamespace->ExecMethod(pathVariable.bstrVal, MethodName, 0, NULL, pInInst,NULL, NULL);
			VariantClear(&pathVariable);
			if (hr != WBEM_S_NO_ERROR) {
				bRet = false;
				goto cleanup;
			}

		}

		// Free up resources
	cleanup:

		SysFreeString(path);
		SysFreeString(ClassPath);
		SysFreeString(MethodName);
		SysFreeString(ArgName0);
		SysFreeString(ArgName1);
		SysFreeString(bstrQuery);

		if (pClass)
			pClass->Release();
		if (pInInst)
			pInInst->Release();
		if (pInClass)
			pInClass->Release();
		if (pLocator)
			pLocator->Release();
		if (pNamespace)
			pNamespace->Release();

		CoUninitialize();

		return bRet;
	}
#else
	// Use the more regular method
	bool Wmi::GetBrightnessInfo(int *brightness, int *levels) {
		GUID *pwrGUID;
		DWORD r = PowerGetActiveScheme(NULL, &pwrGUID);
		UINT32 outVal;
		DWORD size = sizeof(outVal);
		SYSTEM_POWER_STATUS pwr_status;

		if (r != ERROR_SUCCESS) {
			fprintf(stderr, "PowerGetActiveScheme failed: %u\n", r);
			return false;
		}

		GetSystemPowerStatus(&pwr_status);
		bool isDc = pwr_status.ACLineStatus == 0;

		if (isDc) {
			r = PowerReadDCValue(NULL, pwrGUID, &GUID_VIDEO_SUBGROUP, &GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, NULL,
				(LPBYTE)&outVal, &size);
		}
		else {
			r = PowerReadACValue(NULL, pwrGUID, &GUID_VIDEO_SUBGROUP, &GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, NULL,
				(LPBYTE)&outVal, &size);
		}

		if (r != ERROR_SUCCESS) {
			fprintf(stderr, "PowerReadDC/ACValue failed: %u\n", r);
			return false;
		}

		// TODO query properly
		*levels = 100;
		*brightness = outVal;
		return true;
	}

	bool Wmi::SetBrightness(int val) {
		GUID *pwrGUID;
		DWORD r = PowerGetActiveScheme(NULL, &pwrGUID);
		SYSTEM_POWER_STATUS pwr_status;
		if (r != ERROR_SUCCESS) {
			fprintf(stderr, "PowerGetActiveScheme failed: %u\n", r);
			return false;
		}


		GetSystemPowerStatus(&pwr_status);
		bool isDc = pwr_status.ACLineStatus == 0;

		if (isDc) {
			r = PowerWriteDCValueIndex(NULL, pwrGUID, &GUID_VIDEO_SUBGROUP, &GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, val);
		}
		else {
			r = PowerWriteACValueIndex(NULL, pwrGUID, &GUID_VIDEO_SUBGROUP, &GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, val);
		}

		if (r != ERROR_SUCCESS) {
			fprintf(stderr, "PowerWriteDC/ACValueIndex failed: %u\n", r);
			return false;
		}

		r = PowerSetActiveScheme(NULL, pwrGUID);
		if (r != ERROR_SUCCESS) {
			fprintf(stderr, "PowerSetActiveScheme failed: %u\n", r);
			return false;
		}
		return true;
	}

#endif
