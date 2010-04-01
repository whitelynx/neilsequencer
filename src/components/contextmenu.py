#encoding: latin-1

# Aldrin
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Aldrin Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""
This module contains the context menu component for different zzub objects
such as plugins, patterns, and so on. based on the context object currently
selected, items can choose to append themselves or not.
"""

import gtk
import aldrin.common as common
from aldrin.com import com
import zzub

from aldrin.utils import is_generator, is_root, is_controller, is_effect, \
	prepstr, Menu, new_theme_image, gettext
from aldrin.utils import PLUGIN_FLAGS_MASK, ROOT_PLUGIN_FLAGS, \
	GENERATOR_PLUGIN_FLAGS, EFFECT_PLUGIN_FLAGS, CONTROLLER_PLUGIN_FLAGS

class ContextMenu(Menu):
    __aldrin__ = dict(
	    id = 'aldrin.core.contextmenu',
	    singleton = False,
	    categories = [
	    ],
    )

    def __init__(self, contextid, context):
	Menu.__init__(self)
	self.__context_id = contextid
	self.__context = context

    def get_context(self):
	return self.__context

    def get_context_id(self):
	return self.__context_id

    context = property(get_context)
    context_id = property(get_context_id)

    def add_submenu(self, label, submenu = None):
	if not submenu:
	    submenu = ContextMenu(self.__context_id, self.__context)
	return Menu.add_submenu(self, label, submenu)

    def popup(self, parent, event=None):
	for item in self.get_children():
	    item.destroy()
	for item in com.get_from_category('contextmenu.handler'):
	    item.populate_contextmenu(self)
	self.rootwindow = parent
	return Menu.popup(self, parent, event)

class PluginContextMenu(gtk.Menu):
    __aldrin__ = dict(id='aldrin.core.popupmenu', 
                      singleton=True, 
                      categories=['contextmenu.handler'])

    def populate_contextmenu(self, menu):
	if menu.context_id == 'plugin':
	    self.populate_pluginmenu(menu)
	elif menu.context_id == 'connection':
	    self.populate_connectionmenu(menu)
	elif menu.context_id == 'router':
	    self.populate_routermenu(menu)

    def populate_routermenu(self, menu):
        def get_icon_name(self, pluginloader):
            uri = pluginloader.get_uri()
            if uri.startswith('@zzub.org/dssidapter/'):
                return 'dssi'
            if uri.startswith('@zzub.org/ladspadapter/'):
                return 'ladspa'
            if uri.startswith('@psycle.sourceforge.net/'):
                return 'psycle'
            filename = pluginloader.get_name()
            filename = filename.strip().lower()
            for c in '():[]/,.!"\'$%&\\=?*#~+-<>`@ ':
                filename = filename.replace(c, '_')
            while '__' in filename:
                filename = filename.replace('__','_')
            filename = filename.strip('_')
            return filename
        def add_uri(tree, uri, loader):
            if len(uri) == 1:
                tree[uri[0]] = loader
                return tree
            elif uri[0] not in tree:
                tree[uri[0]] = add_uri({}, uri[1:], loader)
                return tree
            else:
                tree[uri[0]] = add_uri(tree[uri[0]], uri[1:], loader)
                return tree
        def populate_from_tree(menu, tree):
            for key, value in tree.iteritems():
                if type(value) is not type({}):
                    menu.add_item(prepstr(key, fix_underscore=True), create_plugin, value)
                else:
                    item, submenu = menu.add_submenu(key)
                    populate_from_tree(submenu, value)
        def create_plugin(item, loader):
            player = com.get('aldrin.core.player')
            player.plugin_origin = menu.context
            player.create_plugin(loader)
        player = com.get('aldrin.core.player')
        plugins = {}
        tree = {}
        item, add_machine_menu = menu.add_submenu("Add machine")
	for pluginloader in player.get_pluginloader_list():
	    plugins[pluginloader.get_uri()] = pluginloader
        for uri, loader in plugins.iteritems():
            type_ = "Generators"
            if loader.get_flags() & zzub.zzub_plugin_flag_has_audio_input:
                type_ = "Effects"
            elif loader.get_flags() & zzub.zzub_plugin_flag_has_event_output:
                type_ = "Controllers"
            author = loader.get_author()
            if len(author) > 20:
                author = author[:20]
            name = loader.get_short_name()
            if len(name) > 20:
                name = name[:20]
            uri_list = [type_, author, name]
            tree = add_uri(tree, uri_list, loader)
        populate_from_tree(add_machine_menu, tree)
        menu.add_separator()
	menu.add_item("Unmute All", self.on_popup_unmute_all)

    def populate_connectionmenu(self, menu):
	mp, index = menu.context
	menu.add_item("Disconnect plugins", self.on_popup_disconnect, mp, index)
	conntype = mp.get_input_connection_type(index)
	if conntype == zzub.zzub_connection_type_audio:
	    # Connection connects two audio plug-ins.
	    pass
	elif conntype == zzub.zzub_connection_type_event:
	    # Connection connects a control plug-in to it's destination.
	    menu.add_separator()
	    mi = mp.get_input_connection_plugin(index).get_pluginloader()
	    for i in range(mi.get_parameter_count(3)):
		param = mi.get_parameter(3, i)
		print param.get_name()

    def populate_pluginmenu(self, menu):
	mp = menu.context
	player = com.get('aldrin.core.player')
	menu.add_check_item("_Mute", common.get_plugin_infos().get(mp).muted, self.on_popup_mute, mp)
	if is_generator(mp):
	    menu.add_check_item("_Solo", player.solo_plugin == mp, self.on_popup_solo, mp)
	menu.add_separator()
	menu.add_item("_Parameters...", self.on_popup_show_params, mp)
	menu.add_item("_Attributes...", self.on_popup_show_attribs, mp)
	menu.add_item("P_resets...", self.on_popup_show_presets, mp)
	menu.add_separator()
	menu.add_item("_Rename...", self.on_popup_rename, mp)
	if not is_root(mp):
	    menu.add_item("_Delete plugin", self.on_popup_delete, mp)
	if is_effect(mp) or is_root(mp):
	    menu.add_separator()
	    menu.add_check_item("Default Target",player.autoconnect_target == mp,self.on_popup_set_target, mp)
	commands = mp.get_commands().split('\n')
	if commands != ['']:
	    menu.add_separator()
	    submenuindex = 0
	    for index in range(len(commands)):
		cmd = commands[index]
		if cmd.startswith('/'):
		    item, submenu = menu.add_submenu(prepstr(cmd[1:], fix_underscore=True))
		    subcommands = mp.get_sub_commands(index).split('\n')
		    submenuindex += 1
		    for subindex in range(len(subcommands)):
			subcmd = subcommands[subindex]
			submenu.add_item(prepstr(subcmd, fix_underscore=True), self.on_popup_command, mp, submenuindex, subindex)
		else:
		    menu.add_item(prepstr(cmd), self.on_popup_command, mp, 0, index)

    def on_popup_rename(self, widget, mp):
	text = gettext(self, "Enter new plugin name:", prepstr(mp.get_name()))
	if text:
	    player = com.get('aldrin.core.player')
	    mp.set_name(text)
	    player.history_commit("rename plugin")

    def on_popup_solo(self, widget, mp):
	"""
	Event handler for the "Mute" context menu option.

	@param event: Menu event.
	@type event: wx.MenuEvent
	"""		
	player = com.get('aldrin.core.player')
	if player.solo_plugin != mp:
	    player.solo(mp)
	else:
	    player.solo(None)

    def on_popup_mute(self, widget, mp):
	"""
	Event handler for the "Mute" context menu option.

	@param event: Menu event.
	@type event: wx.MenuEvent
	"""
	player = com.get('aldrin.core.player')
	player.toggle_mute(mp)		

    def on_popup_delete(self, widget, mp):
	"""
	Event handler for the "Delete" context menu option.
	"""
	player = com.get('aldrin.core.player')
	player.delete_plugin(mp)

    def on_popup_disconnect(self, widget, mp, index):
	"""
	Event handler for the "Disconnect" context menu option.

	@param event: Menu event.
	@type event: wx.MenuEvent
	"""
	plugin = mp.get_input_connection_plugin(index)
	conntype = mp.get_input_connection_type(index)
	mp.delete_input(plugin,conntype)
	player = com.get('aldrin.core.player')
	player.history_commit("disconnect")

    def on_popup_show_attribs(self, widget, mp):
	"""
	Event handler for the "Attributes..." context menu option.

	@param event: Menu event.
	@type event: wx.MenuEvent
	"""
	dlg = com.get('aldrin.core.attributesdialog',mp,self)
	dlg.run()
	dlg.destroy()


    def on_popup_show_presets(self, widget, plugin):
	"""
	Event handler for the "Presets..." context menu option.

	@param event: Menu event.
	@type event: wx.MenuEvent
	"""
	manager = com.get('aldrin.core.presetdialog.manager')
	manager.show(plugin, widget)

    def on_popup_show_params(self, widget, mp):
	"""
	Event handler for the "Parameters..." context menu option.

	@param event: Menu event.
	@type event: wx.MenuEvent
	"""
	manager = com.get('aldrin.core.parameterdialog.manager')
	manager.show(mp, widget)

    def on_popup_new_plugin(self, widget, pluginloader, kargs={}):
	"""
	Event handler for "new plugin" context menu options.
	"""
	player = com.get('aldrin.core.player')
	if 'conn' in kargs:
	    conn = kargs['conn']
	else:
	    conn = None
	if 'plugin' in kargs:
	    plugin = kargs['plugin']
	else:
	    plugin = None
	player.create_plugin(pluginloader, connection=conn, plugin=plugin)

    def on_popup_unmute_all(self, widget):
	"""
	Event handler for unmute all menu option
	"""
	player = com.get('aldrin.core.player')
	for mp in reversed(list(player.get_plugin_list())):
	    info = common.get_plugin_infos().get(mp)
	    info.muted=False
	    mp.set_mute(info.muted)
	    info.reset_plugingfx()

    def on_popup_command(self, widget, plugin, subindex, index):
	"""
	Event handler for plugin commands
	"""
	plugin.command((subindex<<8) | index)

    def on_popup_set_target(self, widget, plugin):
	"""
	Event handler for menu option to set machine as target for default connection
	"""
	self.autoconnect_target = plugin

__aldrin__ = dict(
	classes = [
		ContextMenu,
		PluginContextMenu,
	],
)

