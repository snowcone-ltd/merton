// Globals

const e = React.createElement;

const OPACITY = '0.97';

const MENU_ITEMS = [
	{name: 'System', items: [
		{name: 'load-rom', type: 'action', etype: 'label', label: 'Load ROM'},
		{name: 'unload-rom', type: 'action', etype: 'label', label: 'Unload ROM', needsRunning: true},
		{name: 'reload', type: 'action', etype: 'label', label: 'Reload ROM', needsRunning: true},
		{etype: 'separator'},
		{name: 'reset', type: 'nstate', etype: 'label', label: 'Reset', needsRunning: true},
		{name: 'pause', type: 'nstate', etype: 'checkbox', label: 'Pause Emulation', needsRunning: true},
		{name: 'bg_pause', type: 'cfg', etype: 'checkbox', label: 'Background Pause'},
		{name: 'console', type: 'cfg', etype: 'checkbox', label: 'Console Window'},
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
		{name: 'vsync', type: 'cfg', etype: 'checkbox', label: 'VSync'},
		{name: 'square_pixels', type: 'cfg', etype: 'checkbox', label: 'Square Pixels'},
		{name: 'int_scaling', type: 'cfg', etype: 'checkbox', label: 'Integer Scaling'},
		{etype: 'separator'},
		{name: 'gfx', type: 'cfg', etype: 'dropdown', label: 'Graphics', opts: [
			{label: 'OpenGL', value: 1},
			{label: 'Vulkan', value: 2},
			{label: 'D3D9', value: 3},
			{label: 'D3D11', value: 4},
			{label: 'D3D12', value: 5},
			{label: 'Metal', value: 6},
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


// Menu Actions

function Dropdown(props) {
	const mitem = props.mitem;

	const style = {
		margin: '0 0 1.3rem 0',
		color: `rgba(180, 180, 180, 1.0)`,
		display: 'block',
	};

	let lstyle = {
		textTransform: 'uppercase',
		fontSize: '.7rem',
		fontWeight: 'bold',
	};

	let ddbstyle = {
		margin: '.4rem 0 0 0',
		padding: '.1rem',
		cursor: 'pointer',
		width: '100%',
		borderRadius: '.3rem',
		letterSpacing: '.03rem',
		fontSize: '.9rem',
		fontFamily: 'sans-serif',
		color: '#CCC',
		background: '#666',
	};

	let onChange = (evt) => {
		const index = parseInt(evt.target.selectedIndex);
		const value = mitem.opts[index].value;
		handleEvent({name: mitem.name, type: mitem.type, value: value});
		props.setValue(mitem.type, mitem.name, value);
	};

	if (props.disabled) {
		lstyle.color = 'rgba(120, 120, 120, 1.0)';
		ddbstyle.cursor = '';
		onChange = () => {};
	}

	let sitems = [];

	for (let x = 0; x < mitem.opts.length; x++)
		sitems.push(e('option', {value: mitem.opts[x].value}, mitem.opts[x].label));

	return e('div', {style: style}, [
		e('div', {style: lstyle}, mitem.label),
		e('select', {style: ddbstyle, onChange: onChange, value: props.selected, disabled: props.disabled}, sitems),
	]);
}

function Checkbox(props) {
	const mitem = props.mitem;

	let style = {
		margin: '0 0 1rem 0',
		color: `rgba(180, 180, 180, 1.0)`,
		display: 'block',
	};

	const lstyle = {
		display: 'inline-block',
	};

	let cbstyle = {
		margin: '0 .5rem 0 0',
		padding: '0',
		cursor: 'pointer',
		position: 'relative',
		top: '.1rem',
	};

	let onChange = (evt) => {
		handleEvent({name: mitem.name, type: mitem.type, value: evt.target.checked});
		props.setValue(mitem.type, mitem.name, evt.target.checked);
	};

	if (props.disabled) {
		style.color = `rgba(120, 120, 120, 1.0)`;
		cbstyle.cursor = '';
		onChange = () => {};
	}

	return e('div', {style: style}, [
		e('input', {style: cbstyle, type: 'checkbox', onChange: onChange,
			checked: props.checked, disabled: props.disabled}),
		e('div', {style: lstyle}, mitem.label),
	]);
}

function ActionButton (props) {
	const mitem = props.mitem;

	let style = {
		margin: '0 0 .9rem 0',
		padding: '.1rem',
		cursor: 'pointer',
		background: `rgba(0, 0, 0, 0)`,
		color: `rgba(180, 180, 180, 1.0)`,
		border: 'none',
		display: 'block',
		letterSpacing: 'inherit',
		fontSize: 'inherit',
		fontFamily: 'inherit',
		textAlign: 'inherit',
	};

	let onClick = () =>
		handleEvent({name: mitem.name, type: mitem.type});

	if (props.disabled) {
		style.color = `rgba(120, 120, 120, 1.0)`;
		style.cursor = '';
		onClick = () => {};
	}

	return e('button', {style: style, onClick: onClick, disabled: props.disabled}, mitem.label);
}

function Separator() {
	const style = {
		height: '.025rem',
		width: '100%',
		background: `rgba(100, 100, 100, ${OPACITY})`,
		margin: '0 0 1rem 0',
	};

	return e('div', {style: style});
}


// Menu

function MenuButton(props) {
	let style = {
		cursor: 'pointer',
		padding: '.4rem',
		width: '7rem',
		margin: '.3rem auto 0 auto',
		borderRadius: '.5rem',
		background: `rgba(0, 0, 0, 0)`,
		border: `none`,
		display: 'block',
		color: 'inherit',
		letterSpacing: 'inherit',
		fontSize: 'inherit',
		fontFamily: 'inherit',
		textAlign: 'inherit',
	};

	if (props.selected)
		style.background = `rgba(80, 80, 80, ${OPACITY})`;

	let onClick = () =>
		props.setAppState({menuIndex: props.index});

	if (props.disabled) {
		style.background = 'rgba(0, 0, 0, 0)';
		style.cursor = '';
		style.color = 'rgba(120, 120, 120, 1.0)';
		onClick = () => {};
	}

	return e('button', {style: style, onClick: onClick, disabled: props.disabled}, props.name);
}

function MenuLeft(props) {
	const style = {
		width: '10rem',
		height: '100%',
		paddingTop: '.7rem',
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
		height: '100%',
		minWidth: '13rem',
		padding: '1.4rem',
		background: `rgba(55, 55, 55, ${OPACITY})`,
		borderRight: `.025rem solid rgba(70, 70, 70, ${OPACITY})`,
		boxSizing: 'border-box',
		display: 'inline-block',
		overflowY: 'auto',
	};

	const setValue = (type, key, val) => {
		const obj = props.appState[type];
		obj[key] = val;

		props.setAppState({type: obj});
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
				items.push(e(ActionButton, {mitem: mitem, disabled: disabled}));
				break;
			case 'checkbox':
				const cbval = props.appState[mitem.type][mitem.name];
				items.push(e(Checkbox, {mitem: mitem, setValue: setValue, checked: cbval, disabled: disabled}));
				break;
			case 'dropdown':
				const ddval = mitem.type == 'core_opts' ? props.appState.core_opts[mitem.name].cur :
					props.appState[mitem.type][mitem.name];
				items.push(e(Dropdown, {mitem: mitem, setValue: setValue, selected: ddval, disabled: disabled}));
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

	return e('div', {style: style}, [
		e(MenuLeft, {appState: props.appState, setAppState: props.setAppState}),
		e(MenuRight, {appState: props.appState, setAppState: props.setAppState}),
	]);
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
			menuIndex: 0,
			menuItems: MENU_ITEMS,
			cfg: {},
			core_opts: {},
			nstate: {},
		};

		window.MTY_NativeListener = msg => {
			const json = JSON.parse(msg);

			this.setState(json);

			let menuItems = [...MENU_ITEMS];
			menuItems.push(systemsToMenu());
			menuItems.push(coreOptsToMenu(json.core_opts));

			this.setState({menuItems: menuItems});
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
})();
