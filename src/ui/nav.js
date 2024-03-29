let NAV_CONTROLLER = {};
let NAV_GROUP_INDEX = {};
let NAV_CUR_ELEMENT = null;

let NAV_FOCUS_CLASS = '';
let NAV_CAPTURE_LEFT = null;
let NAV_LOSE_FOCUS = null;
let NAV_CANCEL = null;
let NAV_SCROLL = null


// Helpers

function getNavElements(group) {
	return document.querySelectorAll(`[nav-item='${group}']:not([disabled])`);
}

function getGroup(element) {
	if (!element)
		return -1;

	let group = element.getAttribute('nav-item');

	return group ? parseInt(group) : -1;
}

function getActiveIndex(group, element) {
	let nl = getNavElements(group);

	for (let x = 0; x < nl.length; x++)
		if (element && nl[x].isSameNode(element))
			return x;

	return 0;
}


// Focus elements

function NAV_Focus(group, index) {
	let nl = getNavElements(group);
	if (nl.length == 0)
		return;

	if (index < 0)
		index = 0;

	if (index >= nl.length)
		index = nl.length - 1;

	let element = nl[index];

	if (NAV_CUR_ELEMENT) {
		let prevGroup = getGroup(NAV_CUR_ELEMENT);

		NAV_GROUP_INDEX[prevGroup] = getActiveIndex(prevGroup, NAV_CUR_ELEMENT);
		NAV_CUR_ELEMENT.classList.remove(NAV_FOCUS_CLASS);
	}

	NAV_CUR_ELEMENT = element;
	element.classList.add(NAV_FOCUS_CLASS);
	element.focus();

	if (element.getAttribute('nav-auto'))
		element.click();

	if (NAV_LOSE_FOCUS)
		NAV_LOSE_FOCUS(element);
}

function focusRelative(element, n) {
	let group = getGroup(element);
	if (group == -1)
		group = 0;

	NAV_Focus(group, getActiveIndex(group, element) + n);
}

function NAV_FocusRelative(n) {
	focusRelative(NAV_CUR_ELEMENT, n);
}


// Group manipulation

function NAV_SwitchGroup(group) {
	NAV_Focus(group, NAV_GROUP_INDEX[group] != undefined ? NAV_GROUP_INDEX[group] : 0);
}

function NAV_ResetGroup(group) {
	delete NAV_GROUP_INDEX[group];
}


// Input

function NAV_Input(input) {
	let group = getGroup(NAV_CUR_ELEMENT);

	switch (input) {
		case 'l':
			if (NAV_CAPTURE_LEFT) {
				NAV_CAPTURE_LEFT();

			} else {
				NAV_SwitchGroup(group == -1 ? 0 : group - 1);
			}
			break;
		case 'u':
			NAV_FocusRelative(-1);
			break;
		case 'r':
			NAV_SwitchGroup(group == -1 ? 0 : group + 1);
			break;
		case 'd':
			NAV_FocusRelative(1);
			break;
		case 'a':
			if (NAV_CUR_ELEMENT) {
				NAV_CUR_ELEMENT.click();

				let focusGroup = NAV_CUR_ELEMENT.getAttribute('focus');
				if (focusGroup) {
					NAV_ResetGroup(focusGroup);
					NAV_SwitchGroup(focusGroup);
				}
			}
			break;
		case 'b':
			if (NAV_CANCEL)
				NAV_CANCEL();
			break;
		case 'ls':
			if (NAV_SCROLL)
				NAV_SCROLL(-1);
			break;
		case 'rs':
			if (NAV_SCROLL)
				NAV_SCROLL(1);
			break;
	}
}


// Controllers

let BUTTON_TO;
let BUTTON_REPEAT;

function clearTimers() {
	clearTimeout(BUTTON_TO);
	clearInterval(BUTTON_REPEAT);
}

function inputFirst(json, input) {
	for (let x = 0; x < input.length; x++) {
		if (!NAV_CONTROLLER[input[x]] && json[input[x]]) {
			NAV_Input(input[x])
			break;
		}
	}
}

function noInput(input) {
	for (let x = 0; x < input.length; x++) {
		if (NAV_CONTROLLER[input[x]])
			return false;
	}

	return true;
}

function NAV_Controller(json) {
	let input = ['u', 'd', 'l', 'r'];

	// Don't allow directional input if any other directions are held down already
	if (noInput(input)) {
		clearTimers();

		inputFirst(json, input);

		BUTTON_TO = setTimeout(() => {
			BUTTON_REPEAT = setInterval(() => {
				if (noInput(input))
					clearTimers();

				for (let x = 0; x < input.length; x++) {
					if (NAV_CONTROLLER[input[x]])
						NAV_Input(input[x]);
				}
			}, 40);
		}, 400)
	}

	// Other buttons
	inputFirst(json, ['a', 'b', 'ls', 'rs']);

	NAV_CONTROLLER = json;
}


// Set style, callbacks

function NAV_SetLoseFocus(func) {
	NAV_LOSE_FOCUS = func;
}

function NAV_SetCancel(func) {
	NAV_CANCEL = func;
}

function NAV_SetScroll(func) {
	NAV_SCROLL = func;
}

function NAV_CaptureLeft(func) {
	NAV_CAPTURE_LEFT = func;
}

function NAV_SetFocusClass(focusClass) {
	NAV_FOCUS_CLASS = focusClass;
}


// Attach listeners

window.addEventListener('mouseup', (e) => {
	let group = getGroup(e.srcElement);

	if (group != -1 && e.srcElement.getAttribute('disabled') == null) {
		focusRelative(e.srcElement, 0);

	} else {
		if (NAV_LOSE_FOCUS)
			NAV_LOSE_FOCUS(null);
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
		case 27: // Escape
			e.preventDefault();
			NAV_Input('b');
			break;
	}
});
