# -*- mode: ruby -*-
# vi: set ft=ruby :
#
# Copyright (C) 2013  Maxim Noah Khailo
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# In addition, as a special exception, the copyright holders give 
# permission to link the code of portions of this program with the 
# OpenSSL library under certain conditions as described in each 
# individual source file, and distribute linked combinations 
# including the two.
#
# You must obey the GNU General Public License in all respects for 
# all of the code used other than OpenSSL. If you modify file(s) with 
# this exception, you may extend this exception to your version of the 
# file(s), but you are not obligated to do so. If you do not wish to do 
# so, delete this exception statement from your version. If you delete 
# this exception statement from all source files in the program, then 
# also delete it here.

dir = Dir.pwd
vagrant_dir = File.expand_path(File.dirname(__FILE__))

VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
    config.vm.box = "boxcutter/ubuntu1604"

    config.vm.provider :virtualbox do |vb|
        vb.gui = true
        vb.customize ["modifyvm", :id, "--memory", "2048"]
        vb.customize ["modifyvm", :id, "--cpus", "2"]
        vb.customize ["modifyvm", :id, "--graphicscontroller", "vboxvga"]
        vb.customize ["modifyvm", :id, "--vram", "128"]
        vb.customize ["modifyvm", :id, "--ioapic", "on"]
        vb.customize ["modifyvm", :id, "--hwvirtex", "on"]
        vb.customize ["modifyvm", :id, "--accelerate3d", "on"]
    end

    config.vm.provision :shell, :path => File.join( "vagrant", "bootstrap.sh" )
end
