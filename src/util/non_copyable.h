#ifndef ZYLANN_NON_COPYABLE_H
#define ZYLANN_NON_COPYABLE_H

class NonCopyable {
protected:
	NonCopyable() = default;
	~NonCopyable() = default;

	NonCopyable(NonCopyable const &) = delete;
	void operator=(NonCopyable const &x) = delete;
};

#endif // ZYLANN_NON_COPYABLE_H
