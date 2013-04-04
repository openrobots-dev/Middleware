#include "Middleware.hpp"

Middleware & mw = Middleware::instance();

Node::Node(const char * name) {
	_name = name;
	_next = NULL;
	_publishers = NULL;
	_subscribers = NULL;
	_queued_subscribers = NULL;
}

Node * Node::next(void) {
	return _next;
}

void Node::next(Node * n) {
	_next = n;
}

LocalPublisher * Node::publishers(void) {
	return _publishers;
}

bool Node::advertise(LocalPublisher * pub) {

	if (mw.findLocalPublisher(pub->topic())) {
		return false;
	}

	if (_publishers == NULL) {
		_publishers = pub;
	} else {
		pub->next(_publishers);
		_publishers = pub;
	}

	mw.advertise(pub);

	return true;
}

bool Node::subscribe(LocalSubscriber * sub) {
	LocalPublisher * pub;
	RemotePublisher * rpub;

	pub = mw.findLocalPublisher(sub->topic());

	if (pub) {
		return localSubscribe(pub, sub);
	}

	rpub = mw.findRemotePublisher(sub->topic());

	if (rpub) {
		return remoteSubscribe(rpub, sub);
	}

	queueSubscriber(sub);

	return false;
}

bool Node::localSubscribe(LocalPublisher * pub, LocalSubscriber * sub) {
	LocalSubscriber * p = _subscribers;
	eventid_t eid = 0;

	if (p == NULL) {
		_subscribers = sub;
		sub->subscribe(pub, EVENT_MASK(eid));
		return true;
	}


	// FIXME giusto farlo col sizeof()?
	while ((++eid < sizeof(eventid_t) * 4) && (p->next() != NULL)) {
		// FIXME ?
		p = p->next();
	}

	if (eid == 32) {
		return false;
	}

	p->next(sub);
	sub->subscribe(pub, EVENT_MASK(eid));

	return true;
}

bool Node::remoteSubscribe(RemotePublisher * pub, LocalSubscriber * sub) {
	LocalSubscriber * p = _subscribers;
	eventid_t eid = 0;

	if (p == NULL) {
		_subscribers = sub;
		sub->subscribe(pub, EVENT_MASK(eid));
		return true;
	}


	// FIXME giusto farlo col sizeof()?
	while ((++eid < sizeof(eventid_t) * 4) && (p->next() != NULL)) {
		// FIXME ?
		p = p->next();
	}

	if (eid == 32) {
		return false;
	}

	p->next(sub);
	sub->subscribe(pub, EVENT_MASK(eid));

	return true;
}

LocalSubscriber * Node::queue(void) {
	return _queued_subscribers;
}

void Node::queueSubscriber(LocalSubscriber * sub) {
	if (_queued_subscribers == NULL) {
		_queued_subscribers = sub;
	} else {
		sub->next(_queued_subscribers);
		_queued_subscribers = sub;
	}
}

void Node::unqueueSubscriber(LocalSubscriber * sub) {
	LocalSubscriber *curr_sub, *prev_sub;

	prev_sub = NULL;

	for (curr_sub = _queued_subscribers; curr_sub != NULL;
			prev_sub = curr_sub, curr_sub = curr_sub->next()) {

		if (curr_sub == sub) {
			if (prev_sub == NULL) {
				_queued_subscribers = curr_sub->next();
			} else {
				prev_sub->next(curr_sub->next());
			}
		}
	}

	sub->next(NULL);
}

void Node::spin(systime_t timeout) {
	eventmask_t mask;
	eventid_t eid;
	LocalSubscriber * sub;

	mask = chEvtWaitAnyTimeout(ALL_EVENTS, timeout);

	if (mask == 0) {
		return;
	}

	eid = 0;
	sub = _subscribers;
	while (sub && mask) {
		if (mask & EVENT_MASK(eid)) {
			mask &= ~EVENT_MASK(eid);

			if (sub->callback()) {
				BaseMessage * msg;
				while ((msg = sub->get()) != NULL) {
					sub->callback(msg);
					sub->release(msg);
				}
			}
		}
		eid++;
		sub = sub->next();
	}
}

void Node::spin(void) {
	spin(TIME_INFINITE);
}
