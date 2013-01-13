# Copyright (C) 2012-2013  Ruby-GNOME2 Project Team
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

module GObjectIntrospection
  class Loader
    class << self
      def load(namespace, base_module)
        loader = new(base_module)
        loader.load(namespace)
      end
    end

    def initialize(base_module)
      @base_module = base_module
    end

    def load(namespace)
      repository = Repository.default
      repository.require(namespace)
      pre_load(repository, namespace)
      repository.each(namespace) do |info|
        load_info(info)
      end
      post_load(repository, namespace)
    end

    private
    def pre_load(repository, namespace)
    end

    def post_load(repository, namespace)
    end

    def load_info(info)
      case info
      when FunctionInfo
        load_function_info(info)
      when StructInfo
        load_struct_info(info)
      when EnumInfo
        load_enum_info(info)
      when ObjectInfo
        load_object_info(info)
      when InterfaceInfo
        load_interface_info(info)
      when ConstantInfo
        load_constant_info(info)
      when UnionInfo
        load_union_info(info)
      end
    end

    def load_function_info(info)
      define_module_function(@base_module, info.name, info)
    end

    def define_module_function(target_module, name, function_info)
      target_module.module_eval do
        define_method(name) do |*arguments, &block|
          function_info.invoke(*arguments, &block)
        end
        module_function(name)
      end
    end

    def define_struct(info, options={})
      if info.gtype == GLib::Type::NONE
        klass = self.class.define_struct(info.size, info.name, @base_module,
                                         :parent => options[:parent])
      else
        klass = self.class.define_class(info.gtype, info.name, @base_module,
                                         :parent => options[:parent])
      end
      load_fields(info, klass)
      load_methods(info, klass)
    end

    def load_struct_info(info)
      return if info.gtype_struct?

      define_struct(info)
    end

    def load_enum_info(info)
      self.class.define_class(info.gtype, info.name, @base_module)
    end

    def load_object_info(info)
      klass = self.class.define_class(info.gtype, info.name, @base_module)
      load_fields(info, klass)
      load_methods(info, klass)
    end

    def load_fields(info, klass)
      info.n_fields.times do |i|
        field_info = info.get_field(i)
        name = field_info.name
        flags = field_info.flags

        if flags.readable?
          klass.__send__(:define_method, name) do
            info.get_field_value(self, i)
          end
        end

        if flags.writable?
          klass.__send__(:define_method, "#{name}=") do |value|
            info.set_field_value(self, i, value)
          end
        end
      end
    end

    def load_methods(info, klass)
      grouped_methods = info.methods.group_by do |method_info|
        method_info.class
      end
      grouped_methods.each do |method_info_class, method_infos|
        next if method_infos.empty?
        case method_infos.first
        when ConstructorInfo
          load_constructor_infos(method_infos, klass)
        when MethodInfo
          load_method_infos(method_infos, klass)
        when FunctionInfo
          load_function_infos(method_infos, klass)
        else
          raise "TODO: #{method_info_class}"
        end
      end
    end

    def load_constructor_infos(infos, klass)
      return if infos.empty?

      validate = lambda do |info, arguments|
        validate_arguments(info, arguments)
      end
      infos.each do |info|
        name = "initialize_#{info.name}"
        klass.__send__(:define_method, name) do |*arguments, &block|
          validate.call(info, arguments, &block)
          info.invoke(self, *arguments, &block)
        end
      end

      find_info = lambda do |arguments|
        find_suitable_callable_info(infos, arguments)
      end
      klass.__send__(:define_method, "initialize") do |*arguments, &block|
        info = find_info.call(arguments, &block)
        __send__("initialize_#{info.name}", *arguments, &block)
      end
    end

    def validate_arguments(info, arguments)
      if arguments.size != info.n_in_args
        details = "#{arguments.size} for #{info.n_in_args}"
        raise ArgumentError, "wrong number of arguments (#{detail})"
      end
    end

    def find_suitable_callable_info(infos, arguments)
      min_n_args = nil
      max_n_args = nil
      infos.each do |info|
        if arguments.size == info.n_in_args
          return info
        end
        n_in_args = info.n_in_args
        min_n_args = [min_n_args || n_in_args, n_in_args].min
        max_n_args = [max_n_args || n_in_args, n_in_args].max
      end
      detail = "#{arguments.size} for #{min_n_args}"
      if min_n_args < max_n_args
        detail << "..#{max_n_args}"
      end
      raise ArgumentError, "wrong number of arguments (#{detail})"
    end

    def rubyish_method_name(method_info)
      name = method_info.name
      return_type = method_info.return_type
      if return_type.tag == GObjectIntrospection::TypeTag::BOOLEAN
        case name
        when /\A(?:is|get_is|get)_/
          "#{$POSTMATCH}?"
        when /\A(?:has|use)_/
          "#{name}?"
        else
          name
        end
      elsif /\Aget_/ =~ name and method_info.n_args.zero?
        $POSTMATCH
      else
        name
      end
    end

    def load_method_infos(infos, klass)
      infos.each do |info|
        method_name = rubyish_method_name(info)
        load_method_info(info, klass, method_name)
        if /\Aset_/ =~ method_name and info.n_args == 1
          klass.__send__(:alias_method, "#{$POSTMATCH}=", method_name)
        end
      end
    end

    def load_method_info(info, klass, method_name)
      klass.__send__(:define_method, method_name) do |*arguments, &block|
        info.invoke(self, *arguments, &block)
      end
    end

    def load_function_infos(infos, klass)
      infos.each do |info|
        name = info.name
        next if name == "new"
        next if name == "alloc"
        singleton_class = (class << klass; self; end)
        singleton_class.__send__(:define_method, name) do |*arguments, &block|
          info.invoke(*arguments, &block)
        end
      end
    end

    def load_interface_info(info)
      interface_module =
        self.class.define_interface(info.gtype, info.name, @base_module)
      load_methods(info, interface_module)
    end

    def load_constant_info(info)
      @base_module.const_set(info.name, info.value)
    end

    def load_union_info(info)
      klass = self.class.define_class(info.gtype, info.name, @base_module)
      load_fields(info, klass)
      load_methods(info, klass)
    end
  end
end
