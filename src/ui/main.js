// Globals

const e = React.createElement;

const OPACITY = '0.97';

const MENU_ITEMS = [
	{name: 'System', items: [
		{name: 'load-rom', type: 'action', etype: 'label', label: 'Load ROM'},
		{name: 'unload-rom', type: 'action', etype: 'label', label: 'Unload ROM', needsRunning: true},
		{name: 'reload', type: 'action', etype: 'label', label: 'Reload ROM', needsRunning: true},
		{name: 'reset', type: 'nstate', etype: 'label', label: 'Reset', needsRunning: true},
		{etype: 'separator'},
		{name: 'menu_pause', type: 'cfg', etype: 'checkbox', label: 'Menu Pause'},
		{name: 'bg_pause', type: 'cfg', etype: 'checkbox', label: 'Background Pause'},
		{etype: 'separator'},
		{name: 'disk', type: 'nstate', etype: 'dropdown', label: 'Insert Disk', needsRunning: true, needsDisks: true, opts: [
			{label: 'Select Disk', value: '---'},
			{label: '1', value: 0},
			{label: '2', value: 1},
			{label: '3', value: 2},
		]},
		{name: 'save-state', type: 'nstate', etype: 'dropdown', label: 'Save State', needsRunning: true, opts: [
			{label: 'Select Slot', value: '---'},
			{label: '1', value: 1},
			{label: '2', value: 2},
			{label: '3', value: 3},
			{label: '4', value: 4},
			{label: '5', value: 5},
			{label: '6', value: 6},
			{label: '7', value: 7},
			{label: '8', value: 8},
		]},
		{name: 'load-state', type: 'nstate', etype: 'dropdown', label: 'Load State', needsRunning: true, opts: [
			{label: 'Select Slot', value: '---'},
			{label: '1', value: 1},
			{label: '2', value: 2},
			{label: '3', value: 3},
			{label: '4', value: 4},
			{label: '5', value: 5},
			{label: '6', value: 6},
			{label: '7', value: 7},
			{label: '8', value: 8},
		]},
		{etype: 'separator'},
		{name: 'quit', type: 'action', etype: 'label', label: 'Quit'},
	]},
	{name: 'Video', items: [
		{name: 'reset-window', type: 'action', etype: 'label', label: 'Reset Window'},
		{etype: 'separator'},
		{name: 'fullscreen', type: 'cfg', etype: 'checkbox', label: 'Fullscreen'},
		{name: 'square_pixels', type: 'cfg', etype: 'checkbox', label: 'Square Pixels'},
		{name: 'int_scaling', type: 'cfg', etype: 'checkbox', label: 'Integer Scaling'},
		{etype: 'separator'},
		{name: 'vsync', type: 'cfg', etype: 'dropdown', label: 'VSync', opts: [
			{label: 'Off', value: 0},
			{label: '1.00 (60 Hz)', value: 100},
			{label: '1.25 (75 Hz)', value: 125},
			{label: '2.00 (120 Hz)', value: 200},
			{label: '2.40 (144 Hz)', value: 240},
			{label: '4.00 (240 Hz)', value: 400},
		]},
		{name: 'filter', type: 'cfg', etype: 'dropdown', label: 'Filter', opts: [
			{label: 'Nearest', value: 0},
			{label: 'Linear', value: 1},
		]},
		{name: 'sharpen', type: 'cfg', etype: 'dropdown', label: 'Sharpen', needsLinear: true, opts: [
			{label: 'None', value: 0},
			{label: 'Low', value: 25},
			{label: 'High', value: 75},
		]},
		{name: 'scanlines', type: 'cfg', etype: 'dropdown', label: 'Scanlines', opts: [
			{label: 'Off', value: 0},
			{label: '60%', value: 60},
			{label: '70%', value: 70},
			{label: '80%', value: 80},
			{label: '90%', value: 90},
			{label: '95%', value: 95},
		]},
	]},
	{name: 'Audio', items: [
		{name: 'mute', type: 'cfg', etype: 'checkbox', label: 'Mute'},
		{etype: 'separator'},
		{name: 'audio_buffer', type: 'cfg', etype: 'dropdown', label: 'Audio Buffer', opts: [
			{label: '25 ms', value: 25},
			{label: '50 ms', value: 50},
			{label: '75 ms', value: 75},
			{label: '100 ms', value: 100},
			{label: '125 ms', value: 125},
			{label: '150 ms', value: 150},
		]},
	]},
	{name: 'Advanced', items: [
		{name: 'pause', type: 'nstate', etype: 'checkbox', label: 'Pause Emulation', needsRunning: true},
		{name: 'console', type: 'cfg', etype: 'checkbox', label: 'Console Window'},
		{etype: 'separator'},
		{name: 'gfx', type: 'cfg', etype: 'dropdown', label: 'Graphics', opts: [
			{label: 'OpenGL', value: 1},
			{label: 'Vulkan', value: 2},
			{label: 'D3D11', value: 3},
			{label: 'D3D12', value: 4},
			{label: 'Metal', value: 5},
		]},
		{name: 'playback_rate', type: 'cfg', etype: 'dropdown', label: 'Playback Rate', opts: [
			{label: '44.1 kHz', value: 44100},
			{label: '48 kHz', value: 48000},
			{label: '96 kHz', value: 96000},
		]},
	]},
];

const SYSTEMS = {
	atari2600: {
		name: 'Atari 2600',
		cores: ['stella']
	},
	gameboy: {
		name: 'Game Boy',
		cores: ['gambatte', 'sameboy', 'mgba']
	},
	gba: {
		name: 'Game Boy Advance',
		cores: ['mgba']
	},
	genesis: {
		name: 'Genesis',
		cores: ['genesis-plus-gx', 'picodrive']
	},
	ms: {
		name: 'Master System',
		cores: ['genesis-plus-gx', 'picodrive']
	},
	n64: {
		name: 'N64',
		cores: ['mupen64plus-next', 'parallel-n64']
	},
	nes: {
		name: 'NES',
		cores: ['merton-nes', 'mesen', 'nestopia', 'quicknes']
	},
	ps: {
		name: 'PlayStation',
		cores: ['swanstation', 'mednafen-psx', 'pcsx-rearmed']
	},
	snes: {
		name: 'SNES',
		cores: ['bsnes', 'mesen-s', 'snes9x']
	},
	tg16: {
		name: 'TurboGrafx-16',
		cores: ['mednafen-pce']
	}
};


// Events

function handleEvent(evt) {
	if (window.MTY_NativeSendText)
		window.MTY_NativeSendText(JSON.stringify(evt));
}


// Button

function ActionButton(props) {
	let style = {
		cursor: 'pointer',
		margin: '0.3rem auto 0 auto',
		padding: '.4rem',
		color: `rgba(190, 190, 190, 1.0)`,
		borderRadius: '.5rem',
	};

	if (props.style)
		style = {...style, ...props.style};

	if (props.disabled) {
		style.color = `rgba(120, 120, 120, 1.0)`;
		style.cursor = '';
		onClick = () => {};
	}

	return e('div', {style: style, onClick: props.onClick, disabled: props.disabled,
		'nav-item': props.navGroup, tabindex: props.disabled ? false : -1}, props.label);
}


// Select

function Select(props) {
	const mitem = props.mitem;

	const style = {
		margin: '1.1rem 0 1.2rem 0',
		color: `rgba(190, 190, 190, 1.0)`,
	};

	let lstyle = {
		textTransform: 'uppercase',
		fontSize: '.7rem',
		fontWeight: 'bold',
	};

	let sstyle = {
		background: 'rgba(70, 70, 70, 1)',
		margin: '.4rem 0 0 0',
		padding: '.4rem',
		cursor: 'pointer',
		borderRadius: '.5rem',
		color: '#CCC',
	};

	let onClick = (evt) =>
		props.setAppState({select: {mitem, selected: props.selected}});

	if (props.disabled) {
		sstyle.color = lstyle.color = 'rgba(120, 120, 120, 1.0)';
		sstyle.background = '#444';
		sstyle.cursor = '';
		onChange = () => {};
	}

	let selected = mitem.opts[0].label;

	for (let x = 0; x < mitem.opts.length; x++)
		if (mitem.opts[x].value == props.selected)
			selected = mitem.opts[x].label;

	return e('div', {style: style}, [
		e('div', {style: lstyle}, mitem.label),
		e('div', {style: sstyle, disabled: props.disabled, onClick: onClick, 'nav-item': 1,
			tabindex: props.disabled ? false : -1}, selected),
	]);
}

class SelectMenu extends React.Component {
	constructor(props) {
		super(props);
	}

	componentDidMount() {
		const mitem = this.props.appState.select.mitem;
		const selected = this.props.appState.select.selected;

		for (let x = 0, dummy = 0; x < mitem.opts.length; x++) {
			if (mitem.opts[x].value == '---')
				dummy++;

			if (mitem.opts[x].value == selected) {
				NAV_Focus(2, x - dummy);
				return;
			}
		}

		NAV_Focus(2, 0);
	}

	render() {
		const mitem = this.props.appState.select.mitem;
		const selected = this.props.appState.select.selected;

		const border = `.025rem solid rgba(70, 70, 70, ${OPACITY})`;

		const style = {
			verticalAlign: 'top',
			minWidth: '8rem',
			padding: '.7rem 1rem',
			background: `rgba(55, 55, 55, ${OPACITY})`,
			borderRight: border,
			borderBottom: border,
			boxSizing: 'border-box',
			display: 'inline-block',
			overflowY: 'auto',
		};

		let items = [];

		for (let x = 0; x < mitem.opts.length; x++) {
			if (mitem.opts[x].value == '---')
				continue;

			let style = {};

			if (mitem.opts[x].value == selected) {
				style = {
					color: 'rgba(100, 100, 220, 1)',
					fontWeight: 'bold',
				}
			}

			items.push(e(ActionButton, {
				disabled: false,
				onClick: () => {
					handleEvent({name: mitem.name, type: mitem.type, value: mitem.opts[x].value});
					this.props.setValue(mitem.type, mitem.name, mitem.opts[x].value);
					this.props.setAppState({select: null});
					NAV_SwitchGroup(-1);
				},
				label: mitem.opts[x].label,
				navGroup: 2,
				style: style,
			}));
		}

		return e('div', {style: style}, items);
	}
}


// Checkbox

function Checkbox(props) {
	const mitem = props.mitem;

	let style = {
		cursor: 'pointer',
		margin: '.3rem 0 0 0',
		padding: '.4rem',
		color: `rgba(190, 190, 190, 1.0)`,
		display: 'block',
		borderRadius: '.5rem',
	};

	let cbstyle = {
		margin: '0 .5rem 0 0',
		padding: '0',
		width: '.8rem',
		height: '.8rem',
		position: 'relative',
		pointerEvents: 'none',
		top: '.08rem',
	};

	let onChange = (evt) => {
		handleEvent({name: mitem.name, type: mitem.type, value: evt.target.checked});
		props.setValue(mitem.type, mitem.name, evt.target.checked);
	};

	let onClick = (evt) => {
		evt.preventDefault();

		let input = evt.target.control;

		if (!input.disabled) {
			input.checked = !input.checked;
			onChange({target: input});
		}
	};

	if (props.disabled) {
		style.color = `rgba(120, 120, 120, 1.0)`;
		style.cursor = '';
		onChange = () => {};
	}

	let labelProps = {style: style, onClick: onClick, disabled: props.disabled,
		'nav-item': 1, tabindex: props.disabled ? false : -1};

	let checkboxProps = {style: cbstyle, type: 'checkbox', onChange: onChange,
			checked: props.checked, disabled: props.disabled};

	return e('label', labelProps, e('input', checkboxProps), mitem.label);
}


// Separator

function Separator() {
	const style = {
		height: '.025rem',
		width: '100%',
		background: `rgba(100, 100, 100, ${OPACITY})`,
		margin: '.6rem 0',
	};

	return e('div', {style: style});
}


// Menu

function MenuButton(props) {
	let style = {
		cursor: 'pointer',
		margin: '.3rem auto 0 auto',
		padding: '.4rem',
		borderRadius: '.5rem',
	};

	if (props.selected)
		style.background = `rgba(70, 70, 70, ${OPACITY})`;

	let onClick = () => {
		NAV_ResetGroup(1);
		props.setAppState({menuIndex: props.index});
	}

	if (props.disabled) {
		style.cursor = '';
		style.color = 'rgba(120, 120, 120, 1.0)';
		onClick = () => {};
	}

	return e('div', {style: style, onClick: onClick, disabled: props.disabled,
		'nav-item': 0, 'nav-auto': 1, tabindex: props.disabled ? false : -1}, props.name);
}

function MenuLeft(props) {
	const style = {
		width: '10rem',
		height: '100%',
		padding: '.7rem 1rem',
		background: `rgba(40, 40, 40, ${OPACITY})`,
		borderRight: `.025rem solid rgba(70, 70, 70, ${OPACITY})`,
		boxSizing: 'border-box',
		float: 'left',
	};

	let items = [];

	for (let x = 0; x < props.appState.menuItems.length; x++) {
		const item = props.appState.menuItems[x];
		const disabled = item.needsRunning && !props.appState.nstate.running;
		items.push(e(MenuButton, {index: x, name: item.name, selected: props.appState.menuIndex == x,
			setAppState: props.setAppState, disabled: disabled}));
	}

	return e('div', {style: style}, items);
}

function MenuRight(props) {
	const style = {
		minWidth: '13.5rem',
		height: '100%',
		padding: '.7rem 1.2rem',
		background: `rgba(55, 55, 55, ${OPACITY})`,
		borderRight: `.025rem solid rgba(70, 70, 70, ${OPACITY})`,
		boxSizing: 'border-box',
		display: 'inline-block',
		overflowY: 'auto',
	};

	const menuItems = props.appState.menuItems[props.appState.menuIndex].items;

	let items = [];

	for (let x = 0; x < menuItems.length; x++) {
		const mitem = menuItems[x];
		const disabled = (!props.appState.nstate.running && mitem.needsRunning) ||
			(!props.appState.nstate.has_disks && mitem.needsDisks) ||
			(props.appState.cfg.filter != 1 && mitem.needsLinear);

		switch (mitem.etype) {
			case 'label':
				items.push(e(ActionButton, {
					disabled: disabled,
					onClick: () => handleEvent({name: mitem.name, type: mitem.type}),
					label: mitem.label,
					navGroup: 1,
				}));
				break;
			case 'checkbox':
				const cbval = props.appState[mitem.type][mitem.name];
				items.push(e(Checkbox, {mitem: mitem, setValue: props.setValue, checked: cbval, disabled: disabled}));
				break;
			case 'dropdown':
				const ddval = mitem.type == 'core_opts' ? props.appState.core_opts[mitem.name].cur :
					props.appState[mitem.type][mitem.name];
				items.push(e(Select, {mitem: mitem, setAppState: props.setAppState, selected: ddval, disabled: disabled}));
				break;
			case 'separator':
				items.push(e(Separator));
				break;
		}
	}

	return e('div', {style: style}, items);
}

function Menu(props) {
	const style = {
		width: '100%',
		height: '100%',
		color: 'rgba(230, 230, 230, 1.0)',
		letterSpacing: '.03rem',
		fontSize: '.9rem',
		overflow: 'hidden',
		fontFamily: 'sans-serif',
	};

	const setValue = (type, key, val) => {
		const obj = props.appState[type];
		obj[key] = val;

		props.setAppState({type: obj});
	};

	let items = [
		e(MenuLeft, {appState: props.appState, setAppState: props.setAppState}),
		e(MenuRight, {appState: props.appState, setAppState: props.setAppState, setValue: setValue}),
	];

	if (props.appState.select)
		items.push(e(SelectMenu, {appState: props.appState, setAppState: props.setAppState, setValue: setValue}));

	return e('div', {style: style}, items);
}


// Main

function systemsToMenu() {
	let mrow = {name: 'Cores', items: []};

	const keys = Object.keys(SYSTEMS);

	for (let x = 0; x < keys.length; x++) {
		const system = keys[x];
		const info = SYSTEMS[system];

		let mitem = {name: `core.${system}`, type: 'cfg', etype: 'dropdown', label: info.name, opts: []};

		for (let y = 0; y < info.cores.length; y++) {
			const name = info.cores[y];
			mitem.opts.push({label: name, value: name});
		}

		mrow.items.push(mitem);
	}

	return mrow;
}

function coreOptsToMenu(core_opts) {
	let mrow = {name: 'Core Options', needsRunning: true, items: []};

	mrow.items.push({name: 'core-reset', type: 'action', etype: 'label', label: 'Reset Core Options'});
	mrow.items.push({etype: 'separator'});

	const keys = Object.keys(core_opts);

	for (let x = 0; x < keys.length; x++) {
		const key = keys[x];
		const val = core_opts[key];

		let mitem = {name: key, type: 'core_opts', etype: 'dropdown', label: val.name, opts: []};

		for (let y = 0; y < val.list.length; y++) {
			const opt = val.list[y];
			mitem.opts.push({label: opt, value: opt});
		}

		mrow.items.push(mitem);
	}

	return mrow;
}

class Main extends React.Component {
	constructor(props) {
		super(props);

		this.state = {
			select: null,
			menuIndex: 0,
			menuItems: MENU_ITEMS,
			cfg: {},
			core_opts: {},
			nstate: {},
		};

		NAV_SetLoseFocus((element) => {
			if (element.getAttribute('nav-item') != 2) {
				this.setState({select: null});
				NAV_ResetGroup(2);
			}
		});

		NAV_SetCancel(() => {
			if (this.state.select) {
				this.setState({select: null});
				NAV_ResetGroup(2);
				NAV_SwitchGroup(-1);
			}
		});

		window.MTY_NativeListener = msg => {
			const json = JSON.parse(msg);

			switch (json.type) {
				case 'state': {
					this.setState(json);

					let menuItems = [...MENU_ITEMS];
					menuItems.push(systemsToMenu());
					menuItems.push(coreOptsToMenu(json.core_opts));

					this.setState({menuItems: menuItems});
					break;
				}
				case 'controller': {
					NAV_Controller(json);
					break;
				}
			}
		};
	}

	render() {
		const setAppState = (state) =>
			this.setState(state);

		return e(Menu, {appState: this.state, setAppState: setAppState});
	}
}

(function main() {
	document.oncontextmenu = (e) => {
		e.preventDefault();
		return false;
	};

	window.addEventListener('dragover', (e) => {
		e.preventDefault();
	});

	window.addEventListener('drop', (e) => {
		e.preventDefault();
	});

	ReactDOM.render(e(Main), document.body);

	NAV_Init();
})();
