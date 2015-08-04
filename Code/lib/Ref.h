#pragma once

#ifndef NULL
#define NULL 0
#endif
#define numberof(o)	(sizeof(o) / sizeof((o)[0]))

#define REF_LOGALLOC(obj)
#define REF_LOGRELEASE(obj)

struct RefClass {
	unsigned __ref_count;		// -1 => freed (an unmanaged object will be left at 0)
	RefClass() : __ref_count(0) {}
	~RefClass() { if ((int) __ref_count > 0) throw "Freeing an object that is still retained elsewhere"; }
	// Not allowed if not redefined properly!!! (i.e. not copy the __ref_count)
	RefClass(const RefClass& other);
	RefClass& operator =(const RefClass&other);
};

template<class T>
struct ref {
	T *ptr;

	ref() : ptr(0) {}
	ref(T* eptr) : ptr(eptr) { incref(); }
	ref(T* eptr, bool takeOwnership) : ptr(eptr) { REF_LOGALLOC(eptr); }
	ref(const ref &ref) : ptr(ref.ptr) { incref(); }
	template<class U>
	ref(const ref<U> &ref) : ptr(ref.ptr) { incref(); }
	/************************************************************************/
	/* If you get an error around here, it means that you are trying to use */
	/* a class that is not inheriting RefClass!                             */
	/************************************************************************/
	~ref() { decref(); }

	// Various operator overloading
	ref& operator = (T* eptr) {
		// Not using this logic may free the original object if counter = 1
		incref(eptr);
		decref();
		ptr = eptr;
		return *this;
	}
	template<class U>
	ref<T> &operator = (const ref<U> &ref) { return (*this) = ref.ptr; }
	ref& operator = (const ref &ref) { return (*this) = ref.ptr; }
	T* operator -> () const { return ptr; }
	T& operator * () const { return *ptr; }
	operator T * () const { return ptr; }
	bool operator == (const ref &ref) { return ptr == ref.ptr; }
	bool operator == (const T *eptr) { return ptr == eptr; }

	/**
	 * Call this to get rid of any refs that may delete this object.
	 * Basically this will return a raw pointer to this object and you
	 * will have to delete it manually when you do not need it anymore.
	 * Note that this will render any ref to this object as weak
	 * (potentially dangling once you delete it).
	 */
	T* unmanaged_ptr() { incref(); return ptr; }

private:
	void incref() { if (ptr) ptr->__ref_count++; }
	void incref(T *eptr) { if (eptr) eptr->__ref_count++; }
	void decref() { if (ptr && !ptr->__ref_count--) { REF_LOGRELEASE(ptr); delete ptr; } }
};

/**
 * Call this to create a ref that will be the owner of the object. Other refs can be
 * made out of this object, but once no more ref reference this object, it will be
 * deleted. If you keep pointers to this object elsewhere in your code (not through
 * refs), they will become dangling.
 * Normal use is when creating a new object (use newref which does exactly the same)
 * or when managing an object created as a pointer.
 * A *method() {
 *     return new A;
 * }
 * ref<A> myA = owning_ref(method());
 */
template<class T>
ref<T> owning_ref(T *obj) {
	return ref<T>(obj, true);
}

struct __MkrefHelper {
	template<class T>
	ref<T> operator * (T *detachedRef) {
		return ref<T>(detachedRef, true);
	}
};
#define newref __MkrefHelper() * new 
