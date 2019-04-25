AM_CPPFLAGS  =	-Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align
AM_CPPFLAGS +=	-Wwrite-strings -Wmissing-declarations -Wno-long-long -Werror
AM_CPPFLAGS +=	-Wunused-variable -std=c++11 -D_XOPEN_SOURCE -Os -I.
AM_CPPFLAGS +=	-fno-elide-constructors	 -Weffc++
TEST_UTILS =	test_utils.hh test_utils.cc
CLOC_PSOURCES =	emsha.cc sha256.cc hmac.cc internal.hh			\
		emsha/emsha.hh emsha/hmac.hh emsha/sha256.hh
CLOC_TSOURCES =	 test_emsha.cc test_hmac.cc test_mem.cc test_sha256.cc	\
		 test_utils.cc test_utils.hh
pkgconfigdir =	$(libdir)/pkgconfig
pkgconfig_DATA = libemsha-1.pc

lib_LTLIBRARIES =		libemsha.la
nobase_include_HEADERS =	emsha/sha256.hh emsha/hmac.hh emsha/emsha.hh
libemsha_la_SOURCES =		emsha.cc sha256.cc hmac.cc internal.hh
libemsha_li_CPPFLAGS =		$(AM_CPPFLAGS) -Winline 

check_PROGRAMS =		emsha_sha256_test emsha_hmac_test	\
				emsha_core_test emsha_mem_test		\
				emsha_static_mem_test			\
				emsha_static_sha_test			\
				emsha_static_hmac_test
check_CPPFLAGS =		$(AM_CPPFLAGS) -Wnoinline

# emsha_sha256_test runs through some SHA-256 test vectors, ensuring
# that the library's behaviour is correct.
emsha_sha256_test_SOURCES =	test_sha256.cc $(TEST_UTILS)
emsha_sha256_test_LDADD =	libemsha.la

# emsha_hmac_test runs through a set of HMAC-SHA-256 test vectors,
# ensuring that the library's behaviour is correct.
emsha_hmac_test_SOURCES =	test_hmac.cc $(TEST_UTILS)
emsha_hmac_test_LDADD =		libemsha.la

# emsha_core_test validates some of the additional functions provided
# by the emsha library.
emsha_core_test_SOURCES =	test_emsha.cc $(TEST_UTILS)
emsha_core_test_LDADD =		libemsha.la

# emsha_mem_test is used for testing with valgrind; it aims to introduce
# no heap allocations via the test harness so that memory usage inside
# the library may be more accurately checked.
emsha_mem_test_SOURCES =	test_mem.cc $(TEST_UTILS)
emsha_mem_test_LDADD =		libemsha.la

emsha_static_mem_test_SOURCES =	test_mem.cc emsha.cc sha256.cc hmac.cc $(TEST_UTILS)
emsha_static_mem_test_CPPFLAGS = $(AM_CPPFLAGS) -static
emsha_static_mem_test_LDFLAGS = $(AM_LDFLAGS) -static

emsha_static_sha_test_SOURCES =	test_sha256.cc emsha.cc sha256.cc hmac.cc $(TEST_UTILS)
emsha_static_sha_test_CPPFLAGS = $(AM_CPPFLAGS) -static
emsha_static_sha_test_LDFLAGS = $(AM_LDFLAGS) -static

emsha_static_hmac_test_SOURCES =	test_hmac.cc emsha.cc sha256.cc hmac.cc $(TEST_UTILS)
emsha_static_hmac_test_CPPFLAGS =	$(AM_CPPFLAGS) -static
emsha_static_hmac_test_LDFLAGS =	$(AM_LDFLAGS) -static


.PHONY: valgrind-check
valgrind-check: emsha_static_mem_test
	valgrind --tool=massif -v emsha_static_mem_test ms_print

.PHONY: cloc-report
cloc-report:
	@echo "=== Library Sources ==="
	@cloc $(CLOC_PSOURCES)
	@echo
	@echo "=== Test Sources ==="
	@cloc $(CLOC_TSOURCES)
	
.PHONY:	coverity-scan
coverity-scan: clean
	cov-build --dir cov-int make all check
	tar czf $(PACKAGE_NAME)-$(PACKAGE_VERSION)_coverity.tar.gz cov-int
	rm -rf cov-int

.PHONY: scanners clang-scanner cppcheck-scanner
scanners: clang-scanner cppcheck-scanner
clang-scanner:
	clang++ $(AM_CPPFLAGS) --analyze $(CLOC_PSOURCES)

cppcheck-scanner:
	cppcheck --quiet --enable=all -I ./ $(CLOC_PSOURCES)


