include projects.make

ifndef LIBRARIES
$(error "LIBRARIES must be defined.")
endif

QUIET   = @
targets = all clean cleanall unit-test install docs
paths   = src unit-tests

target_deps = $(foreach path,$(paths),$(addprefix $(1)-,$(addsuffix /$(path),$(LIBRARIES))))
all_deps = $(foreach target,$(targets),$(call target_deps,$(target)))

.DEFAULT : install
.PHONY : $(targets) $(all_deps)

install ::

define LIBRARIES_template
$(3)-$(1)/$(2) :
	$(QUIET)echo "---- $(1)/$(2) - $(3) ----------------------------------------"
	$(QUIET)make --no-print-directory -C $(1)/$(2) -f Makefile $3
endef

define TARGET_template
$(1) :: $$(call target_deps,$(1))
endef

create_path_targets = $(foreach library,$(LIBRARIES),$(eval $(call LIBRARIES_template,$(library),$(1),$(2))))
create_targets      = $(foreach path,$(paths),$(eval $(call create_path_targets,$(path),$(1))))

$(foreach target,$(targets),$(eval $(call TARGET_template,$(target))))

$(foreach target,$(targets),$(eval $(call create_targets,$(target))))

cleanall ::
	$(QUIET)rm -rf ../bins

