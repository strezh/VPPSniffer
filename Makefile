all::
		$(MAKE) -C parportvirt
		$(MAKE) -C parportsnif
		$(MAKE) -C sniftest
		@echo "***** Driver Compiled *****"

clean::
		$(MAKE) -C parportvirt clean
		$(MAKE) -C parportsnif clean
		$(MAKE) -C sniftest clean

insert:: parportvirt/parportvirt.ko parportsnif/parportsnif.ko
		/sbin/insmod parportvirt/parportvirt.ko; sleep 1
		@echo ****** parportvirt loaded *****
		/sbin/insmod  parportsnif/parportsnif.ko; sleep 1
		@echo ****** parportsnif loaded *****
		@echo "***** Drivers Loaded *****"

remove::
		/sbin/rmmod parportvirt.ko; sleep 1
		/sbin/rmmod parportsnif.ko; sleep 1;
		@echo "***** Driver Unloaded *****"

