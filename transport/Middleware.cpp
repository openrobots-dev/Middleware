#include "Middleware.hpp"

Middleware Middleware::_instance;
Config::Transport & Middleware::_transport = Config::Transport::instance();

Middleware::Middleware(void) {
	_nodes = NULL;
}

Middleware & Middleware::instance(void) {
	return _instance;
}

Config::Transport & Middleware::transport(void) {
	return _transport;
}

void Middleware::newNode(Node * n) {
	Node * node;

	if (_nodes == NULL ) {
		_nodes = n;
		return;
	}

	node = _nodes;

	while (node->next() != NULL) {
		node = node->next();
	}

	node->next(n);
}

void Middleware::delNode(Node * node) {
	Node *curr_node, *prev_node;

	prev_node = NULL;

	for (curr_node = _nodes; curr_node != NULL;
			prev_node = curr_node, curr_node = curr_node->next()) {

		if (curr_node == node) {
			if (prev_node == NULL) {
				_nodes = curr_node->next();
			} else {
				prev_node->next(curr_node->next());
			}
		}
	}

	node->next(NULL);
}

bool Middleware::advertise(LocalPublisher * pub) {
	Node * node;
	LocalSubscriber * waiting_subscriber;

	node = _nodes;

	while (node) {
		waiting_subscriber = node->queue();

		while (waiting_subscriber != NULL) {
			if (compareTopics(pub->topic(),
					waiting_subscriber->topic())) {
				node->unqueueSubscriber(waiting_subscriber);
				node->localSubscribe(pub, waiting_subscriber);
			}
			waiting_subscriber = (LocalSubscriber *) waiting_subscriber->next();
		}

		node = node->next();
	}

	chSysLock();
	chSchRescheduleS();
	chSysUnlock();

	return true;
}


bool Middleware::advertise(RemotePublisher * pub) {
	Node * node;
	LocalSubscriber * waiting_subscriber;

	if (_remote_publishers == NULL) {
		_remote_publishers = pub;
	} else {
		pub->next(_remote_publishers);
		_remote_publishers = pub;
	}

	node = _nodes;

	while (node) {
		waiting_subscriber = node->queue();

		while (waiting_subscriber != NULL) {
			if (compareTopics(pub->topic(),
					waiting_subscriber->topic())) {
				node->unqueueSubscriber(waiting_subscriber);
				node->remoteSubscribe(pub, waiting_subscriber);
			}
			waiting_subscriber = (LocalSubscriber *) waiting_subscriber->next();
		}

		node = node->next();
	}

	chSysLock();
	chSchRescheduleS();
	chSysUnlock();

	return true;
}

/*
 bool BaseMiddleware::subscribe(BaseSubscriber * sub, systime_t timeout) {
 BasePublisher * pub;
 msg_t msg;

 pub = findPublisher(sub->topic());

 if (pub == NULL) {
 if (timeout == 0) {
 return false;
 }

 if (subscribers_queue == NULL) {
 subscribers_queue = sub;
 } else {
 sub->setNext(subscribers_queue);
 subscribers_queue = sub;
 }

 chSysLock();
 chSchGoSleepTimeoutS(THD_STATE_SLEEPING, timeout);
 msg = chThdSelf()->p_u.rdymsg;
 chSysUnlock();
 fanculo
 if (msg == RDY_TIMEOUT) {
 return false;
 } else {
 pub = (BasePublisher *) msg;
 }
 lol
 }

 sub->source = pub;

 if (subscribers == NULL) {
 subscribers = sub;
 } else {
 sub->setNext(subscribers);
 subscribers = sub;
 }

 return true;
 }
 */

bool_t Middleware::compareTopics(const char * t1, const char * t2) {
	int cc = 0;

	while (cc == 0 && *t1 && *t2)
		cc = *t1++ - *t2++;

	if (cc == 0 && *t1 == 0 && *t2 == 0) {
		return true;
	} else {
		return false;
	}
}

LocalPublisher * Middleware::findLocalPublisher(const char * topic) {
	Node * node;
	LocalPublisher * pub;

	node = _nodes;

	while (node) {
		pub = node->publishers();

		while (pub != NULL) {
			if (compareTopics(topic, pub->topic())) {
				return pub;
			}

			pub = pub->next();
		}
		node = node->next();
	}

	return NULL;
}

RemotePublisher * Middleware::findRemotePublisher(const char * topic) {
	RemotePublisher * pub;

	pub = _remote_publishers;

	while (pub != NULL) {
		if (compareTopics(topic, pub->topic())) {
			return pub;
		}
		pub = pub->next();
	}

	return NULL;
}

