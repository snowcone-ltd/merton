let NAV_GROUP = 0
let NAV_LAST_ELEMENT = {}
let NAV_CONTROLLER = {}

function getNavElements(group) {
	return document.querySelectorAll(`[nav-item='${group}']:not([disabled])`);
}

function getActiveIndex(nl) {
	for (let x = 0; x < nl.length; x++)
		if (nl[x].isSameNode(NAV_LAST_ELEMENT[NAV_GROUP]))
			return x;

	return 0;
}

function focusElement(group, n) {
	let nl = getNavElements(group);
	let i = getActiveIndex(nl);

	if (i + n < nl.length && i + n >= 0) {
		let node = nl[i + n];
		node.focus();

		if (node.getAttribute('nav-auto'))
			node.click();

		NAV_LAST_ELEMENT[group] = node;
	}
}

function switchGroup(group, n) {
	let nl = getNavElements(NAV_GROUP + n);

	if (nl.length > 0)
		NAV_GROUP += n;

	if (NAV_LAST_ELEMENT[NAV_GROUP]) {
		NAV_LAST_ELEMENT[NAV_GROUP].focus();

	} else {
		focusElement(NAV_GROUP, 0);
	}
}

function NAV_ResetGroup(group) {
	delete NAV_LAST_ELEMENT[group];
}

function NAV_Init() {
	let style = document.createElement('style');

	style.appendChild(document.createTextNode(`
		div:focus, label:focus{
			background-color: rgba(80, 80, 80, 1) !important;
		}

		div:focus, label:focus, select:focus {
			outline: .12rem solid rgba(160, 160, 160, 1);
		}
	`));

	document.head.appendChild(style);

	let nl = getNavElements(0);

	if (nl[0])
		nl[0].focus();
}

function NAV_Input(input) {
	switch (input) {
		case 'l':
			switchGroup(NAV_GROUP, -1);
			break;
		case 'u':
			focusElement(NAV_GROUP, -1);
			break;
		case 'r':
			switchGroup(NAV_GROUP, 1);
			break;
		case 'd':
			focusElement(NAV_GROUP, 1);
			break;
		case 'a':
			NAV_LAST_ELEMENT[NAV_GROUP].click()
			NAV_LAST_ELEMENT[NAV_GROUP].focus()
			break;
		case 'b':
			break;
	}
}

function inputFirst(json, input) {
	for (let x = 0; x < input.length; x++) {
		if (!NAV_CONTROLLER[input[x]] && json[input[x]]) {
			NAV_Input(input[x])
			break;
		}
	}
}

function NAV_Controller(json) {
	let input = ['u', 'd', 'l', 'r'];
	let noButtons = true;

	// Don't allow directional input if any other directions are held down already
	for (let x = 0; x < input.length; x++) {
		if (NAV_CONTROLLER[input[x]]) {
			noButtons = false;
			break;
		}
	}

	// Move on button press
	if (noButtons)
		inputFirst(json, input);

	// A, B buttons
	inputFirst(json, ['a', 'b']);

	NAV_CONTROLLER = json;
}

window.addEventListener('mousedown', (e) => {
	let group = e.srcElement.getAttribute('nav-item');

	if (group) {
		NAV_GROUP = parseInt(group);
		NAV_LAST_ELEMENT[NAV_GROUP] = e.srcElement;

	} else {
		e.preventDefault();
	}
});

window.addEventListener('keydown', (e) => {
	switch (e.which) {
		case 9:  // Tab
		case 32: // Space
			e.preventDefault();
			break;
		case 13: // Enter
			e.preventDefault();
			NAV_Input('a');
			break;
		case 37: // Left
			e.preventDefault();
			NAV_Input('l');
			break;
		case 38: // Up
			e.preventDefault();
			NAV_Input('u');
			break;
		case 39: // Right
			e.preventDefault();
			NAV_Input('r');
			break;
		case 40: // Down
			e.preventDefault();
			NAV_Input('d');
			break;
	}
});
