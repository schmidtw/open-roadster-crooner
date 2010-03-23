include projects.make

ifndef LIBRARIES
$(error "LIBRARIES must be defined.")
endif

QUIET   = @
build_dirs   = src mock ut

targets_src  = all clean cleanall install docs listings
paths_src    = src

targets_ut   = all clean unit-test
paths_ut     = unit-tests

paths_mock   = mock
targets_mock = all clean cleanall install


# 1 - target -- all clean
# 2 - directory -- src mock unit-tests
target_deps = $(addprefix $(1)-,$(addsuffix /$(2),$(LIBRARIES)))

all_deps = $(foreach dir,$(build_dirs),$(foreach target,$(targets_$(dir)),$(call target_deps,$(target),$(paths_$(dir)))))
all_targets = $(sort $(foreach dir,$(build_dirs),$(targets_$(dir))))

.DEFAULT_GOAL : install
.PHONY : $(all_targets) $(all_deps)

install ::
#	@echo "all_deps -- $(all_deps)"

# 1 - library name
#   ex: linked-list
# 2 - directory
#   ex: src
# 3 - target
#   ex: clean
define LIBRARIES_template
$(3)-$(1)/$(2) :
	$(QUIET)echo "---- $(1)/$(2) - $(3) ----------------------------------------"
	$(QUIET)make --no-print-directory -C $(1)/$(2) -f Makefile $3
endef

# 1 - target
#   ex: clean
# 2 - build_dir
#   ex: unit-test mock src
define TARGET_template
$(1) :: $$(call target_deps,$(1),$(2))
endef

# 1 - directory
#   ex: src,mock,unit-test
# 2 - target
#   ex: clean
create_path_targets = $(foreach library,$(LIBRARIES),$(eval $(call LIBRARIES_template,$(library),$(1),$(2))))

# 1 - target
#   ex: clean
# 2 - dir
#   ex: ut
create_targets      = $(foreach path,$(paths_$(2)),$(eval $(call create_path_targets,$(path),$(1))))

$(foreach dir,$(build_dirs),$(foreach target,$(targets_$(dir)),$(eval $(call TARGET_template,$(target),$(paths_$(dir))))))

$(foreach dir,$(build_dirs),$(foreach target,$(targets_$(dir)),$(eval $(call create_targets,$(target),$(dir)))))

clean ::
	$(QUIET)rm -rf html lcov.out

cleanall ::
	$(QUIET)rm -rf ../bins html lcov.out

coverage :: unit-test
	$(QUIET)lcov -q -d . -c --output-file lcov.out
	$(QUIET)lcov -q --output-file lcov.out -r lcov.out */unit-tests/*
	$(QUIET)genhtml -q -o html lcov.out
